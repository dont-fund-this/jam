#include <regex>

// Test embedded filesystem library
#include <iostream>
#include <dlfcn.h>
#include "embedded_file.h"

extern const EmbeddedFile embedded_data[];

using InvokeFn = const char* (*)(const char*, const char*, const char*);

int main() {
    std::cout << "=== TESTING EFS LIBRARY ===" << std::endl;
    
    // Load the efs library
    void* handle = dlopen("../../dist/libefs.dylib", RTLD_LAZY);
    if (!handle) {
        std::cerr << "Failed to load libefs.dylib: " << dlerror() << std::endl;
        return 1;
    }

    // Get Invoke function
    InvokeFn invoke = (InvokeFn)dlsym(handle, "Invoke");
    if (!invoke) {
        std::cerr << "Failed to find Invoke function" << std::endl;
        dlclose(handle);
        return 1;
    }

    // Test: list all embedded files
    std::cout << "\n=== TEST 1: List embedded files ===" << std::endl;
    const char* result = invoke("efs.list", "{}", "{}");
    std::cout << "Result: " << (result ? result : "null") << std::endl;

    // Test: read specific file
    std::cout << "\n=== TEST 2: Read embedded file ===" << std::endl;
    result = invoke("efs.read", R"({"path":"docs/structure.md"})", "{}");
    std::cout << "Result: " << (result ? result : "null") << std::endl;

    // Test: read another file
    std::cout << "\n=== TEST 3: Read c++-reference.md ===" << std::endl;
    result = invoke("efs.read", R"({"path":"docs/c++-reference.md"})", "{}");
    std::cout << "Result: " << (result ? result : "null") << std::endl;

    // Test: non-existent file
    std::cout << "\n=== TEST 4: Read non-existent file ===" << std::endl;
    result = invoke("efs.read", R"({"path":"docs/notfound.md"})", "{}");
    std::cout << "Result: " << (result ? result : "null") << std::endl;

    // Test: binary file (should be base64 encoded)
    std::cout << "\n=== TEST 5: Read binary file (test.bin) ===" << std::endl;
    result = invoke("efs.read", R"({"path":"test.bin"})", "{}");
    std::cout << "Result: " << (result ? result : "null") << std::endl;

    // Print all embedded file contents using efs.list and efs.read
    std::cout << "\n=== TEST 6: Print all embedded file contents ===" << std::endl;
    const char* list_json = invoke("efs.list", "{}", "{}");
    std::string list_result = list_json ? list_json : "";
    size_t files_arr = list_result.find("\"files\"");
    if (files_arr != std::string::npos) {
        size_t arr_start = list_result.find('[', files_arr);
        size_t arr_end = list_result.find(']', arr_start);
        if (arr_start != std::string::npos && arr_end != std::string::npos) {
            std::string files_array = list_result.substr(arr_start, arr_end - arr_start + 1);
            std::regex re("\"([^\"]+)\"");
            auto begin = std::sregex_iterator(files_array.begin(), files_array.end(), re);
            auto end = std::sregex_iterator();
            int count = 0;
            for (auto it = begin; it != end; ++it) {
                std::string fname = (*it)[1].str();
                std::string payload = std::string("{\"path\":\"") + fname + "\"}";
                const char* res = invoke("efs.read", payload.c_str(), "{}");
                std::cout << "File [" << (++count) << "]: " << fname << std::endl;
                if (res) {
                    std::cout << res << std::endl;
                } else {
                    std::cout << "null" << std::endl;
                }
                std::cout << std::endl;
            }
            std::cout << "Total files printed: " << count << std::endl;
        }
    }
    dlclose(handle);
    std::cout << "\n=== TESTS COMPLETE ===" << std::endl;
    return 0;
}
