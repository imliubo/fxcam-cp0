#include <cassert>
#include <memory>

#include "screens/screen.h"

namespace {

class CountingViewModel final : public viewmodel::BaseViewModel {
 public:
  void on_enter() override { ++enter_count; }
  void on_exit() override { ++exit_count; }

  int enter_count{0};
  int exit_count{0};
};

}  // namespace

int main() {
  auto viewmodel = std::make_shared<CountingViewModel>();
  {
    screen::Screen screen(nullptr, nullptr, viewmodel);
    screen.show();
    screen.show();
    assert(viewmodel->enter_count == 1);

    screen.hide();
    screen.hide();
    assert(viewmodel->exit_count == 1);

    screen.show();
    assert(viewmodel->enter_count == 2);
  }
  assert(viewmodel->exit_count == 2);
  return 0;
}
