// Simple base64 encode wrapper for jam/libs/efs
#include <string>
#include "../../_other_/llama.cpp/common/base64.hpp"

inline std::string base64_encode(const unsigned char* data, size_t size) {
    std::string output;
    base64::encode(data, data + size, std::back_inserter(output));
    return output;
}
