#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PRESET="${PRESET:-cp0-cross}"
CONFIGURATION="${CONFIGURATION:-Release}"

cmake --preset "${PRESET}" "$@"
cmake --build --preset "${PRESET}-rel"

BUILD_DIR="${ROOT_DIR}/build/${PRESET}"
CONTROL_FILE="${BUILD_DIR}/generated/package/control"
if [[ ! -f "${CONTROL_FILE}" ]]; then
  echo "Package metadata was not generated in ${BUILD_DIR}" >&2
  exit 1
fi

control_value() {
  sed -n "s/^${1}: //p" "${CONTROL_FILE}" | head -n 1
}

PACKAGE="$(control_value Package)"
VERSION="$(control_value Version)"
ARCHITECTURE="$(control_value Architecture)"
if [[ -z "${PACKAGE}" || -z "${VERSION}" || -z "${ARCHITECTURE}" ]]; then
  echo "Incomplete Debian package metadata in ${CONTROL_FILE}" >&2
  exit 1
fi

WORK_DIR="$(mktemp -d "${TMPDIR:-/tmp}/fxcam-package.XXXXXX")"
trap 'rm -rf "${WORK_DIR}"' EXIT
DATA_DIR="${WORK_DIR}/data"
CONTROL_DIR="${WORK_DIR}/control"
mkdir -p "${DATA_DIR}/usr" "${CONTROL_DIR}" "${ROOT_DIR}/dist"

cmake --install "${BUILD_DIR}" --config "${CONFIGURATION}" --prefix "${DATA_DIR}/usr"

invalid_paths=()
while IFS= read -r staged_path; do
  relative_path="${staged_path#${DATA_DIR}/}"
  case "${relative_path}" in
    usr/share/APPLaunch/* | usr/share/doc/* | usr/share/"${PACKAGE}"/* | \
      usr/lib/"${PACKAGE}"/* | opt/"${PACKAGE}"/*)
      ;;
    *)
      invalid_paths+=("${relative_path}")
      ;;
  esac
done < <(find "${DATA_DIR}" \( -type f -o -type l \) -print)

if (( ${#invalid_paths[@]} > 0 )); then
  echo "Package contains files outside CardputerZero portal install prefixes:" >&2
  printf '  %s\n' "${invalid_paths[@]}" >&2
  exit 1
fi

if ! find "${DATA_DIR}/usr/share/APPLaunch/applications" -type f -name '*.desktop' \
  -print -quit | grep -q .; then
  echo "Package is missing usr/share/APPLaunch/applications/*.desktop" >&2
  exit 1
fi

PACKAGE_NAME="${PACKAGE}_${VERSION}_${ARCHITECTURE}.deb"
PACKAGE_PATH="${ROOT_DIR}/dist/${PACKAGE_NAME}"

if cpack --help 2>/dev/null | grep -Eq '^  DEB[[:space:]]'; then
  cmake --build "${BUILD_DIR}" --config "${CONFIGURATION}" --target package
  if [[ ! -f "${PACKAGE_PATH}" ]]; then
    echo "Expected package was not generated: ${PACKAGE_PATH}" >&2
    exit 1
  fi
  echo "Created ${PACKAGE_PATH}"
  exit 0
fi

cp "${CONTROL_FILE}" "${CONTROL_DIR}/control"
printf '2.0\n' > "${WORK_DIR}/debian-binary"

if tar --version 2>&1 | grep -q 'bsdtar'; then
  TAR_FORMAT="gnutar"
else
  TAR_FORMAT="gnu"
fi

COPYFILE_DISABLE=1 tar --format="${TAR_FORMAT}" --no-xattrs \
  --uid 0 --gid 0 --uname root --gname root \
  -cJf "${WORK_DIR}/control.tar.xz" -C "${CONTROL_DIR}" .
COPYFILE_DISABLE=1 tar --format="${TAR_FORMAT}" --no-xattrs \
  --uid 0 --gid 0 --uname root --gname root \
  -cJf "${WORK_DIR}/data.tar.xz" -C "${DATA_DIR}" .

(
  cd "${WORK_DIR}"
  ar -rcS "${PACKAGE_NAME}" debian-binary control.tar.xz data.tar.xz
)
mv "${WORK_DIR}/${PACKAGE_NAME}" "${PACKAGE_PATH}"
echo "Created ${PACKAGE_PATH} (portable ar/tar fallback; CPack DEB generator unavailable)"
