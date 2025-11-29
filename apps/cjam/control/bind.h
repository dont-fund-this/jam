#pragma once

#include <cstddef>

using DispatchFn = const char* (*)(const char* address, const char* payload, const char* options);
using AttachFn = bool (*)(DispatchFn dispatch, char* err_buf, std::size_t err_cap);
using DetachFn = bool (*)(char* err_buf, std::size_t err_cap);
using InvokeFn = const char* (*)(const char* address, const char* payload, const char* options);

struct ControlFns {
    AttachFn attach;
    DetachFn detach;
    InvokeFn invoke;
};

bool control_bind(void* handle, ControlFns* fns);
