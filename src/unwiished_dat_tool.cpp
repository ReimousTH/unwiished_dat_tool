// unwiished_dat_tool.cpp: определяет точку входа для приложения.
//

#include "unwiished_dat_tool.h"
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <xxhash.h>

using namespace std;

std::vector<std::string> path_to_;
bool ignore_hash_system;

//utility
uint32_t CalculateFileHashXXHash(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) return 0;;

    XXH32_state_t* state = XXH32_createState();
    XXH32_reset(state, 0);

    char buffer[4096];
    while (file.read(buffer, sizeof(buffer)) || file.gcount()) {
        XXH32_update(state, buffer, file.gcount());
    }

    XXH32_hash_t hash = XXH32_digest(state);
    XXH32_freeState(state);

    return hash;
}

std::string mode_convert(std::string path)
{
    DatFile dat;
    dat.open(path.c_str(), GENERIC_READ, OPEN_EXISTING);
    dat.ReadFile();
    dat.close();

    std::string json_file_path = std::filesystem::path(path).replace_extension(L".json").string();

    FFile json_file;
    if (!json_file.open(json_file_path.c_str(),GENERIC_WRITE,CREATE_ALWAYS)) {
        Debug::Error("Failed to create JSON file");
        return "";
    }


    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    writer.SetIndent(' ', 2);

    rapidjson::Document doc;
    dat.Serialize(doc, doc.GetAllocator());
    doc.Accept(writer);

    json_file.writeString(buffer.GetString());
    json_file.close();


    Debug::Info("Successfully converted %s to %s", path.c_str(), std::filesystem::path(json_file_path).string().c_str());

    return json_file_path;
}




void ProcessEntry(DirectoryEntry& entry, std::string path)
{
    std::filesystem::create_directories(path);

    std::string files_list_path = path + "\\!files.txt";
    std::string files_hash_path = path + "\\!hash.txt";
    std::ofstream files_list(files_list_path);
    std::ofstream files_list_hash(files_hash_path);

    if (!files_list.is_open()) {
        Debug::Error("Failed to create !files.txt in: %s", path.c_str());
        return;
    }

    for (auto& dir : entry.directories)
    {
        files_list << "[DIR] " << dir.name << "\n";
        auto current_dir_path = path + "\\" + dir.name;
        ProcessEntry(dir, current_dir_path);
    }

    for (auto& file : entry.files)
    {
        uint32_t hash_one_two = 0;
        auto size_onz_one_real = file.resourceOneSize;
        auto size_one = file.resourceOneSize;
        auto size_two = file.resourceTwoSize;

        auto file_path = path + "\\" + file.name;
        file_path = std::filesystem::path(file_path).replace_extension(".onz").string();

        if (std::filesystem::exists(file_path))
        {
            hash_one_two = CalculateFileHashXXHash(file_path);
            Debug::Info("%x : %x : %x : %x", hash_one_two, size_onz_one_real, size_one, size_two);
        }
        else
        {
            Debug::Info(Debug::LevelH_Medium,"File does not exist: %s", file_path.c_str());
            Debug::Info(Debug::LevelH_Medium,"%x : %x : %x : %x", -1, size_onz_one_real, size_one, size_two);
        }

        // Write to files
        files_list << file.name << "\n";
        files_list_hash << hash_one_two << "\n";

        Debug::Info(Debug::LevelH_Medium,"Processing file: %s", file_path.c_str());
    }

    files_list.close();
    files_list_hash.close();
}


int mode_read(std::string path)
{
    DatFile dat;
    dat.open(path.c_str(), GENERIC_READ, OPEN_EXISTING);
    dat.ReadFile();
    dat.close();
    ProcessEntry(dat.root, std::filesystem::path(path).parent_path().string());
    return 1;
}


//path = load_main_game/stage012/

void ProcessSingleFile(FileEntry& file, std::string file_path, uint32_t padding_value, uint32_t stored_hash = 0) {

    file_path = std::filesystem::path(file_path).replace_extension(".onz").string();
    if (!std::filesystem::exists(file_path)) {
        Debug::Info(Debug::LevelH_Medium, "File does not exist: %s", file_path.c_str());
        return;
    }

    uint32_t current_hash = CalculateFileHashXXHash(file_path);
    bool hash_changed = (current_hash != stored_hash);

    if ((hash_changed || ignore_hash_system) && (stored_hash != 0 || path_to_.size() > 0)) {
        Debug::Info("File changed, updating: %s", file_path.c_str());
        FFile onz_file;
        onz_file.open(file_path.c_str(), GENERIC_READ | OPEN_EXISTING, OPEN_EXISTING);

        // LZ11 Check (.dat variant) - PRESERVED LOGIC
        if (file.resourceOneSize != 0 && file.resourceTwoSize != 0) {
            struct LZ11Header {
                uint32_t data;
                uint8_t getCompressionType() const { return data & 0xFF; }
                uint32_t getDecompressedSize() const { return (data >> 8) & 0xFFFFFF; }
            };

            auto one_size = onz_file.read<LZ11Header>().getDecompressedSize();
            auto onz_size = onz_file.size();
            file.resourceOneSize = Padding::CalculateSizeForOne<uint32_t>(one_size, onz_size, padding_value);
            file.resourceTwoSize = Padding::CalculateSizeForOnz<uint32_t>(onz_size);

            //why?
            if (file.resourceOneSize == 0 || file.resourceTwoSize == 0)
            {
                Debug::Info("ASFSf");
            }
        }
        else
        {

        }

        onz_file.close();
    }

    // Update file sizes with padding calculation
    auto file_size = std::filesystem::file_size(file_path);
    auto calculated_size = Padding::CalculateSizeForOnz<uint32_t>(file_size);

    if (calculated_size != file.resourceTwoSize) {
        // Debug::Warning("Wrong Padding for %s ", file_path.c_str());
    }

    file.resourceOneSize = 0;
    file.resourceTwoSize = calculated_size;

    Debug::Info(hash_changed ? Debug::LevelH_Low : Debug::LevelH_Medium,
        "Processing file: %s (changed: %s)", file_path.c_str(), hash_changed ? "yes" : "no");
}

// Unified function with hash file support for batch processing
void ProcessFiles(DirectoryEntry& entry, const std::string& base_path,
    const std::string& target_file = "", uint32_t padding_value = 0) {

    // For single file processing (fast path)
    if (!target_file.empty()) {
        for (auto& file : entry.files) {
            if (file.name == target_file) {
                auto file_path = base_path + "\\" + target_file;
                ProcessSingleFile(file, file_path, padding_value);
                return;
            }
        }
        Debug::Warning("File not found: %s", target_file.c_str());
        return;
    }

    // For batch processing with hash files (original logic)
    std::string files_hash_path = base_path + "\\!hash.txt";
    std::vector<uint32_t> existing_hashes;

    // Read existing hashes
    std::ifstream hash_input(files_hash_path);
    if (hash_input.is_open()) {
        std::string line;
        while (std::getline(hash_input, line)) {
            if (!line.empty()) {
                existing_hashes.push_back(std::stoul(line, nullptr, 10));
            }
        }
        hash_input.close();
    }

    // Process all files with hash tracking
    std::ofstream files_list_hash(files_hash_path);
    size_t index = 0;

    for (auto& file : entry.files) {
        uint32_t stored_hash = (index < existing_hashes.size()) ? existing_hashes[index] : 0;
        auto file_path = base_path + "\\" + file.name;
        file_path = std::filesystem::path(file_path).replace_extension(".onz").string();

        ProcessSingleFile(file, file_path, padding_value, stored_hash);
        files_list_hash << CalculateFileHashXXHash(file_path) << "\n";
        index++;
    }
    files_list_hash.close();

    // Process subdirectories recursively
    for (auto& dir : entry.directories) {
        auto dir_path = base_path + "\\" + dir.name;
        std::filesystem::create_directories(dir_path);
        ProcessFiles(dir, dir_path, "", padding_value);
    }
}

// Simple walk function with error handling
DirectoryEntry* walk_to_entry(DirectoryEntry& entry, const std::string& path) {
    DirectoryEntry* current = &entry;

    for (const auto& component : std::filesystem::path(path)) {
        std::string name = component.string();
        if (name.empty() || name == ".") continue;

        bool found = false;
        for (auto& dir : current->directories) {
            if (dir.name == name) {
                current = &dir;
                found = true;
                break;
            }
        }
        if (!found) return current;
    }
    return current;
}

// Clean mode_build function with LZ11 support
int mode_build(const std::string& dat_path, uint32_t padding_value) {
    DatFile dat;
    dat.open(dat_path.c_str(), GENERIC_READ | GENERIC_WRITE, OPEN_EXISTING);
    dat.ReadFile();

    auto root_dir = std::filesystem::path(dat_path).parent_path().string();

    for (const auto& path_entry : path_to_) {
        auto relative_path = std::filesystem::path(path_entry).lexically_relative(root_dir).string();

        if (std::filesystem::is_regular_file(path_entry)) {
            // Process single file (fast)
            auto dir_path = std::filesystem::path(path_entry).parent_path().string();
            auto file_name = std::filesystem::path(path_entry).replace_extension(".one").filename().string();

            if (auto* entry = walk_to_entry(dat.root, relative_path))
            {
                ProcessFiles(*entry, dir_path, file_name, padding_value);
            }
        }
        else {
            // Process entire directory (with hash tracking)
            if (auto* entry = walk_to_entry(dat.root, relative_path)) {
                ProcessFiles(*entry, path_entry, "", padding_value);
            }
        }
    }
    ProcessFiles(dat.root, root_dir, "", padding_value);

    dat.jump(0);
    dat.WriteFile();
    dat.close();
    return 1;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        Debug::Info("Usage: unleashed_dat_tool <path_to_file> [--build\--unpack\..e.g]");
        Debug::Info("  path_to_file - path to .dat file");
        Debug::Info("  --unpack - .dat to .json");
        Debug::Info("  --build - create dat file from json (not implemented yet)");
        Debug::Info("  --padding=value - add extra padding manually, for example area.onz(for original 1248) and any common_dump.onz(here arround 200000 - 375000)");
       
        Debug::Info("  --path = \".path_1,path_2 ... .\"(not yet)");
        Debug::Info("  --local_padding = \".path_1,path_2 ... .\"(not yet) - for path only");

        Debug::Info("  --force - ignore hash system (not yet)");
        Debug::Info("example # 1 --build --padding=1024");
        return 1;
    }

    std::string input_path = argv[1];

    if (!std::filesystem::exists(input_path)) {
        Debug::Error("File not found: %s", input_path.c_str());
        return 1;
    }

    bool build_mode = false;
    bool build_read = false;
    bool build_pack = false;
    bool build_unpack = false;
    uint32_t padding_value = 0;

   

    for (int i = 2; i < argc; i++) {

        std::string arg = argv[i];
        if (arg == "--build")
        {
            build_mode = true;
            continue;
        }

        if (arg == "--read")
        {
            build_read = true;
            continue;
        }
        if (arg == "--pack")
        {
            build_pack = true;
            continue;
        }
        if (arg == "--unpack")
        {
            build_unpack = true;
            continue;
        }

        if (arg.find("--path") == 0) 
        {
            std::string pathValue = arg.substr(7);

        
            std::stringstream ss(pathValue);
            std::string path;

            while (std::getline(ss, path, ',')) 
            {
                path_to_.push_back(path);
            }

        }
        if (arg == "--force")
        {
            ignore_hash_system = true;
            continue;
        }

        if (arg.find("--padding") == 0)
        {
            sscanf(argv[i], "--padding=%d", &padding_value);
        }


    }

    if (build_unpack)
        mode_convert(input_path);

    if (build_read)
        mode_read(input_path);

    if (build_mode)
        mode_build(input_path, padding_value);

}