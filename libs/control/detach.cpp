#include "contract.h"
#include "registry.h"
#include <iostream>

extern DispatchFn g_dispatch;

extern "C" bool Detach(char* /* err_buf */, std::size_t /* err_cap */) {
    g_dispatch = nullptr;
    
    // Clean up any loaded plugins in registry
    control_cleanup_registry();
    
    std::cout << "CONTROL: Detach() called" << std::endl;
    return true;
}
