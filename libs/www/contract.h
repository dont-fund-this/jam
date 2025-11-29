#pragma once

#include <cstddef>

struct libsinfo {
    const char* plugin_type;
    const char* product;
    const char* description_long;
    const char* description_short;
    unsigned long plugin_id;
};

// Dispatch callback - allows plugins to invoke other plugins via host
typedef const char* (*DispatchFn)(const char* address, const char* payload, const char* options);

extern "C" {
    bool Attach(DispatchFn dispatch, char* err_buf, std::size_t err_cap);
    bool Detach(char* err_buf, std::size_t err_cap);
    bool Report(char* err_buf, std::size_t err_cap, libsinfo* out);
    const char* Invoke(const char* address, const char* payload, const char* options);
}
