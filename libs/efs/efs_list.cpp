#include "handler.h"
#include <cstring>

// External symbols from embedded_refs.cpp (auto-generated)
extern "C" {
    extern const char* embedded_files[];
}

handler_def efs_list_with() {
    return {
        .sid = "efs.list",
        .tag = "embedded",
        .fun = [](const char* /* payload */, const char* /* options */, std::string& /* err */) -> std::any {
            std::string result = R"({"success":true,"files":[)";
            
            for (int i = 0; embedded_files[i] != nullptr; i++) {
                if (i > 0) result += ",";
                result += "\"";
                result += embedded_files[i];
                result += "\"";
            }
            
            result += "]}";
            return result;
        }
    };
}
