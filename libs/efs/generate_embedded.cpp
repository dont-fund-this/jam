// Generate embedded_refs.cpp from refs/ directory
#include <iostream>
#include <fstream>
#include <filesystem>
#include <set>

namespace fs = std::filesystem;

int main() {
    const fs::path refs_dir = "../../refs";
    const fs::path output_file = "embedded_refs.cpp";
    
    std::set<fs::path> ref_files;
    for (const auto& entry : fs::recursive_directory_iterator(refs_dir)) {
        if (entry.is_regular_file() && entry.path().filename() != ".DS_Store") {
            ref_files.insert(fs::relative(entry.path(), refs_dir.parent_path()));
        }
    }
    
    if (ref_files.empty()) {
        std::cerr << "ERROR: No files found in " << refs_dir << std::endl;
        return 1;
    }
    
    std::cout << "Generating " << output_file << " with " << ref_files.size() << " files..." << std::endl;
    
    std::ofstream out(output_file);
    out << "// Auto-generated from refs/ directory\n";
    out << "#define INCBIN_SILENCE_BITCODE_WARNING\n";
    out << "#include \"incbin.h\"\n\n";
    
    for (const auto& file : ref_files) {
        std::string path = file.string();
        std::string identifier = path;
        for (char& c : identifier) if (!std::isalnum(c)) c = '_';
        out << "INCBIN(ref_" << identifier << ", \"../../" << path << "\");\n";
    }
    
    out << "\nconst char* embedded_files[] = {\n";
    for (const auto& file : ref_files) {
        std::string path = file.string();
        if (path.starts_with("refs/")) path = path.substr(5);
        out << "    \"" << path << "\",\n";
    }
    out << "    nullptr\n};\n\n";
    
    out << "struct EmbeddedFile {\n";
    out << "    const char* path;\n";
    out << "    const unsigned char* data;\n";
    out << "    unsigned int size;\n";
    out << "};\n\n";
    
    out << "extern \"C\" const EmbeddedFile embedded_data[] = {\n";
    for (const auto& file : ref_files) {
        std::string path = file.string();
        std::string display = path;
        if (display.starts_with("refs/")) display = display.substr(5);
        std::string identifier = path;
        for (char& c : identifier) if (!std::isalnum(c)) c = '_';
        out << "    {\"" << display << "\", gref_" << identifier << "Data, gref_" 
            << identifier << "Size},\n";
    }
    out << "    {nullptr, nullptr, 0}\n};\n";
    
    out.close();
    std::cout << "Generated successfully" << std::endl;
    return 0;
}
