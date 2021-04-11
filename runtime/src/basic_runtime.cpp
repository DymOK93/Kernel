#include <basic_runtime.h>
#include <crt_runtime.h>
#include <heap.h>

namespace ktl::crt {
static handler_t destructor_stack[128] = {
    nullptr};  // C++ Standard requires 32 or more
static unsigned int destructor_count{0};
static driver_unload_t custom_driver_unload{nullptr};

static constexpr unsigned int MAX_DESTRUCTOR_COUNT{sizeof(destructor_stack) /
                                            sizeof(handler_t)};
static constexpr int INIT_CODE = 0x4b544caU;
static constexpr int EXIT_CODE = 0x4b544ceU;

void CRTCALL doexit(_In_ int) noexcept {
  if (ktl::crt::destructor_count) {
    do {
      auto& destructor{
          ktl::crt::destructor_stack[--ktl::crt::destructor_count]};  // index =
                                                                      // size -
                                                                      // 1
      destructor();
    } while (ktl::crt::destructor_count);
  }
}

int CRTCALL cinit(_In_ int) noexcept {  //����� �������������
  for (ktl::crt::handler_t* ctor_ptr = __cxx_ctors_begin__;
       ctor_ptr < __cxx_ctors_end__; ++ctor_ptr) {
    auto& constructor{*ctor_ptr};
    if (constructor) {
      constructor();
    }
  }
  return 0;
}
}  // namespace ktl::crt

int CRTCALL atexit(ktl::crt::handler_t destructor) {
  if (destructor &&
      ktl::crt::destructor_count < ktl::crt::MAX_DESTRUCTOR_COUNT) {
    ktl::crt::destructor_stack[ktl::crt::destructor_count++] = destructor;
    return 0;
  }
  return 1;
}

NTSTATUS KtlDriverEntry(PDRIVER_OBJECT driver_object,
                        PUNICODE_STRING registry_path) noexcept {
  ktl::crt::cinit(ktl::crt::INIT_CODE);
  NTSTATUS init_status{DriverEntry(driver_object, registry_path)};
  if (!NT_SUCCESS(init_status)) {
    KtlDriverUnload(driver_object);
  } else {
    ktl::crt::custom_driver_unload = driver_object->DriverUnload;
    driver_object->DriverUnload = &KtlDriverUnload;
  }
  return init_status;
}

void KtlDriverUnload(PDRIVER_OBJECT driver_object) noexcept {
  if (ktl::crt::custom_driver_unload) {
    ktl::crt::custom_driver_unload(driver_object);
  }
  ktl::crt::doexit(ktl::crt::EXIT_CODE);
}