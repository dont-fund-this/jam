#include "handler.h"
#include <cstring>




#include "embedded_file.h"
// External symbols from embedded_refs.cpp (auto-generated)
extern "C" {
    extern const char* embedded_files[];
    extern const EmbeddedFile embedded_data[];
}


#include "handler.h"
#include <cstring>
#include <string>
#include <iostream>
#include <sstream>

// External symbols from embedded_refs.cpp (auto-generated)
extern "C" {
    extern const char* embedded_files[];
    extern const EmbeddedFile embedded_data[];
}

handler_def efs_read_with() {
    return {
        .sid = "efs.read",
        .tag = "embedded",
        .fun = [](const char* payload, const char* /* options */, std::string& /* err */) -> std::any {
            // Parse payload for "path"
            std::string path;
            if (payload) {
                std::string p(payload);
                size_t pos = p.find("\"path\":");
                if (pos != std::string::npos) {
                    pos = p.find('"', pos + 7);
                    size_t end = p.find('"', pos + 1);
                    if (pos != std::string::npos && end != std::string::npos)
                        path = p.substr(pos + 1, end - pos - 1);
                }
            }
            if (path.empty()) {
                return R"({"success":false,"error":"no path specified"})";
            }
            // Find file and return content
            for (int i = 0; embedded_data[i].path != nullptr; i++) {
                if (std::strcmp(embedded_data[i].path, path.c_str()) == 0) {
                    std::string content((const char*)embedded_data[i].data, embedded_data[i].size);
                    std::ostringstream oss;
                    oss << "{\"success\":true,\"name\":\"" << path << "\",\"content\":\"";
                    // Escape quotes and control characters for JSON
                    for (const char& c : content) {
                        if (c == '"') oss << "\\\"";
                        else if (c == '\\') oss << "\\\\";
                        else if (c == '\n') oss << "\\n";
                        else if (c == '\r') oss << "\\r";
                        else if (c == '\t') oss << "\\t";
                        else if ((unsigned char)c < 0x20) oss << "?";
                        else oss << c;
                    }
                    oss << "\"}";
                    return oss.str();
                }
            }
            return R"({"success":false,"error":"file not found"})";
        }
    };
}
