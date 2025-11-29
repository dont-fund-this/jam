#pragma once

#include "contract.h"
#include <string>
#include <vector>

using DispatchFn = const char* (*)(const char* address, const char* payload, const char* options);
using AttachFn = bool (*)(DispatchFn dispatch, char* err_buf, std::size_t err_cap);
using DetachFn = bool (*)(char* err_buf, std::size_t err_cap);
using InvokeFn = const char* (*)(const char* address, const char* payload, const char* options);
using ReportFn = bool (*)(char* err_buf, std::size_t err_cap, libsinfo* out);

struct LoadedPlugin {
    std::string name;
    void* handle;
    AttachFn attach;
    DetachFn detach;
    InvokeFn invoke;
    ReportFn report;
};

// Global plugin registry access
std::vector<LoadedPlugin>& control_get_registry();

// Registry operations
void control_cleanup_registry();
bool control_discover_and_load(DispatchFn dispatch);
