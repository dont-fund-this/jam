#include "control/boot.h"
#include "control/bind.h"
#include "control/attach.h"
#include "control/invoke.h"
#include "control/detach.h"
#include <iostream>

int main(int /* argc */, char** argv) {
    std::cout << "=== MINIMAL HOST ===" << std::endl;

    void* handle = control_boot(argv);
    if (!handle) return 1;

    ControlFns fns;
    if (!control_bind(handle, &fns)) return 1;
    
    if (!control_attach(handle, fns)) return 1;

    control_invoke(fns);
    control_detach(handle, fns);

    std::cout << "=== MINIMAL HOST COMPLETE ===" << std::endl;
    return 0;
}
