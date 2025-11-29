#include "contract.h"
#include <iostream>

DispatchFn g_dispatch = nullptr;

extern "C" bool Attach(DispatchFn dispatch, char* /* err_buf */, std::size_t /* err_cap */) {
    g_dispatch = dispatch;
    std::cout << "EFS: Attach() called" << std::endl;
    return true;
}
