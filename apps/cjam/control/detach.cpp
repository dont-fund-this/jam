#include "detach.h"
#include "bind.h"
#include <iostream>

#if defined(__APPLE__)
    #include <dlfcn.h>
    #define LIB_CLOSE(handle) dlclose(handle)
#elif defined(__linux__)
    #include <dlfcn.h>
    #define LIB_CLOSE(handle) dlclose(handle)
#elif defined(_WIN32)
    #include <windows.h>
    #define LIB_CLOSE(handle) FreeLibrary((HMODULE)handle)
#endif

void control_detach(void* handle, const ControlFns& fns) {
    char detach_err[256] = {0};
    if (!fns.detach(detach_err, sizeof(detach_err))) {
        std::cerr << "Warning: Control plugin Detach failed: " << detach_err << std::endl;
    }
    LIB_CLOSE(handle);
}
