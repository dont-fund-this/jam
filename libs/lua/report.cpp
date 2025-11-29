#include "contract.h"

extern "C" bool Report(char* /* err_buf */, std::size_t /* err_cap */, libsinfo* out) {
    if (out == nullptr) {
        return false;
    }
    out->plugin_type = "lua";
    out->product = "plugin";
    out->description_long = "LUA plugin";
    out->description_short = "lua";
    out->plugin_id = 0x00000000UL;
    return true;
}
