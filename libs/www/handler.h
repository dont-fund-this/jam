#pragma once

#include <any>
#include <functional>
#include <initializer_list>
#include <string>
#include <vector>

using handler_fn = std::function<std::any(const char*, const char*, std::string&)>;

struct handler_def {
    std::string sid;
    std::string tag;
    handler_fn fun;
};

using handler_list = std::vector<handler_def>;

inline handler_list spread(std::initializer_list<handler_list> lists) {
    handler_list result;
    for (const auto& list : lists) {
        result.insert(result.end(), list.begin(), list.end());
    }
    return result;
}
