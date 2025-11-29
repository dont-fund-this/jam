#include "bind.h"
#include <cstddef>
#include <iostream>

#if defined(__APPLE__)
    #include <dlfcn.h>
    #define LIB_SYM(handle, name) dlsym(handle, name)
    #define LIB_CLOSE(handle) dlclose(handle)
#elif defined(__linux__)
    #include <dlfcn.h>
    #define LIB_SYM(handle, name) dlsym(handle, name)
    #define LIB_CLOSE(handle) dlclose(handle)
#elif defined(_WIN32)
    #include <windows.h>
    #define LIB_SYM(handle, name) GetProcAddress((HMODULE)handle, name)
    #define LIB_CLOSE(handle) FreeLibrary((HMODULE)handle)
#endif

bool control_bind(void* handle, ControlFns* fns) {
    fns->attach = (AttachFn)LIB_SYM(handle, "Attach");
    fns->detach = (DetachFn)LIB_SYM(handle, "Detach");
    fns->invoke = (InvokeFn)LIB_SYM(handle, "Invoke");

    if (!fns->attach || !fns->detach || !fns->invoke) {
        std::cerr << "Error: Control plugin missing required functions" << std::endl;
        LIB_CLOSE(handle);
        return false;
    }

    std::cout << "Resolved control plugin functions (Attach/Detach/Invoke)" << std::endl;
    return true;
}
