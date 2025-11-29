#include "contract.h"
#include <iostream>

extern DispatchFn g_dispatch;

extern "C" bool Detach(char* /* err_buf */, std::size_t /* err_cap */) {
    g_dispatch = nullptr;
    std::cout << "EGE: Detach() called" << std::endl;
    return true;
}
