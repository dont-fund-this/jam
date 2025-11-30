// Simple base64 decode wrapper for jam/libs/llm
#include <string>
#include "../../_other_/llama.cpp/common/base64.hpp"

inline std::string base64_decode(const std::string& input) {
    std::string output;
    base64::decode(input.begin(), input.end(), std::back_inserter(output));
    return output;
}
