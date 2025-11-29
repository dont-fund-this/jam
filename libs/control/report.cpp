#include "contract.h"
#include <exception>
#include <cstring>

static constexpr const char* kPluginType = "control";
static constexpr const char* kProduct = "core";
static constexpr const char* kDescLong = "Control plugin for discovery and orchestration";
static constexpr const char* kDescShort = "control";
static constexpr unsigned long kPluginId = 0x00000000UL; // Reserved ID for control

extern "C" bool Report(char* err_buf, std::size_t err_cap, libsinfo* out) {
    if (out == nullptr) {
        if (err_buf && err_cap > 0) {
            std::strncpy(err_buf, "descriptor handle is null", err_cap - 1);
            err_buf[err_cap - 1] = '\0';
        }
        return false;
    }

    try {
        out->plugin_type = kPluginType;
        out->product = kProduct;
        out->description_long = kDescLong;
        out->description_short = kDescShort;
        out->plugin_id = kPluginId;
        if (err_buf && err_cap > 0) {
            err_buf[0] = '\0';
        }
        return true;
    } catch (const std::exception& ex) {
        if (err_buf && err_cap > 0) {
            std::strncpy(err_buf, ex.what(), err_cap - 1);
            err_buf[err_cap - 1] = '\0';
        }
    } catch (...) {
        if (err_buf && err_cap > 0) {
            std::strncpy(err_buf, "unknown report failure", err_cap - 1);
            err_buf[err_cap - 1] = '\0';
        }
    }

    return false;
}
