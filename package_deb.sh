#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PRESET="${PRESET:-cp0-cross}"
CONFIGURATION="${CONFIGURATION:-Release}"

cmake --preset "${PRESET}" "$@"
cmake --build --preset "${PRESET}-rel"

BUILD_DIR="${ROOT_DIR}/build/${PRESET}"
if cpack --help 2>/dev/null | grep -Eq '^  DEB[[:space:]]'; then
  cmake --build "${BUILD_DIR}" --config "${CONFIGURATION}" --target package
  exit 0
fi

CACHE_FILE="${BUILD_DIR}/CMakeCache.txt"
CONTROL_FILE="${BUILD_DIR}/generated/package/control"
if [[ ! -f "${CACHE_FILE}" || ! -f "${CONTROL_FILE}" ]]; then
  echo "Package metadata was not generated in ${BUILD_DIR}" >&2
  exit 1
fi

cache_value() {
  sed -n "s/^${1}:[^=]*=//p" "${CACHE_FILE}" | tail -n 1
}

VERSION="$(cache_value FXCAM_VERSION)"
REVISION="$(cache_value APP_DEBIAN_REVISION)"
ARCHITECTURE="$(cache_value APP_DEBIAN_ARCHITECTURE)"
if [[ -z "${VERSION}" || -z "${REVISION}" || -z "${ARCHITECTURE}" ]]; then
  echo "Incomplete Debian package metadata in ${CACHE_FILE}" >&2
  exit 1
fi

WORK_DIR="$(mktemp -d "${TMPDIR:-/tmp}/fxcam-package.XXXXXX")"
trap 'rm -rf "${WORK_DIR}"' EXIT
DATA_DIR="${WORK_DIR}/data"
CONTROL_DIR="${WORK_DIR}/control"
mkdir -p "${DATA_DIR}/usr" "${CONTROL_DIR}" "${ROOT_DIR}/dist"

cmake --install "${BUILD_DIR}" --config "${CONFIGURATION}" --prefix "${DATA_DIR}/usr"
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

PACKAGE_NAME="FXCam_${VERSION}_${REVISION}_${ARCHITECTURE}.deb"
PACKAGE_PATH="${ROOT_DIR}/dist/${PACKAGE_NAME}"
(
  cd "${WORK_DIR}"
  ar -rcS "${PACKAGE_NAME}" debian-binary control.tar.xz data.tar.xz
)
mv "${WORK_DIR}/${PACKAGE_NAME}" "${PACKAGE_PATH}"
echo "Created ${PACKAGE_PATH} (portable ar/tar fallback; CPack DEB generator unavailable)"
