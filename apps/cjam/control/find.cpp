#include "control-find.h"
#include "control-boot.h"
#include <filesystem>
#include <iostream>

void* control_find(char** argv) {
    std::filesystem::path exe_path = std::filesystem::canonical(argv[0]);
    std::filesystem::path exe_dir = exe_path.parent_path();

    void* handle = control_boot(exe_dir);
    if (!handle) {
        std::cerr << "Error: No control plugin found in " << exe_dir << std::endl;
    }
    return handle;
}
