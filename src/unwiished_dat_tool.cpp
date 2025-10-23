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
std::map<std::string,uint32_t> GLocalPadding;
bool ignore_hash_system;
uint32_t gGlobalPadding = 0;

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

    dat.open(path.c_str(), std::ios::in | std::ios::binary);
    dat.ReadFile();
    dat.close();

    std::string json_file_path = std::filesystem::path(path).replace_extension(L".json").string();

    FFile json_file;
    if (!json_file.open(json_file_path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc)) {
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




void ProcessEntry_Cache(DirectoryEntry* entry, std::string path)
{
    std::filesystem::create_directories(path);

    for (auto& dir : entry->directories)
    {
        auto current_dir_path = path + "\\" + dir->name;
        ProcessEntry_Cache(dir.get(), current_dir_path);
    }

    for (auto& file : entry->files)
    {
        uint32_t hash_one_two = 0;
        auto size_onz_one_real = file->resourceOneSize;
        auto size_one = file->resourceOneSize;
        auto size_two = file->resourceTwoSize;

        auto file_path = path + "\\" + file->name;
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

        file->resourceOneSize = hash_one_two;
        file->resourceTwoSize = 1128350536;

        Debug::Info(Debug::LevelH_Medium,"Processing file: %s", file_path.c_str());
    }
}


int mode_read(std::string path)
{
    DatFile dat;
    dat.open(path.c_str(), std::ios::in | std::ios::binary);
    dat.ReadFile();
    dat.close();
    auto cache_path = std::filesystem::path(path).replace_extension("cbin").string();
    dat.open(cache_path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
    ProcessEntry_Cache(dat.root.get(),std::filesystem::path(path).parent_path().string());
    dat.WriteFile(); //Generate Cache File
    return 1;
}

//path = load_main_game/stage012/
void ProcessSingleFile(FileEntry* file, std::string file_path, uint32_t padding_value, uint32_t stored_hash = 0) {

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
        onz_file.open(file_path.c_str(), std::ios::in | std::ios::binary);

        // LZ11 Check (.dat variant) - PRESERVED LOGIC
        if (file->resourceOneSize != 0 && file->resourceTwoSize != 0) {
            struct LZ11Header {
                uint32_t data;
                uint8_t getCompressionType() const { return data & 0xFF; }
                uint32_t getDecompressedSize() const { return (data >> 8) & 0xFFFFFF; }
            };

            auto one_size = onz_file.read<LZ11Header>().getDecompressedSize();
            auto onz_size = onz_file.size();
            file->resourceOneSize = Padding::CalculateSizeForOne<uint32_t>(one_size, onz_size, padding_value);
            file->resourceTwoSize = Padding::CalculateSizeForOnz<uint32_t>(onz_size);
        }
        else
        {
            file->resourceOneSize = 0;
            file->resourceTwoSize = Padding::CalculateSizeForOnz<uint32_t>(onz_file.size());
        }

        onz_file.close();
    }

    // Update file sizes with padding calculation
    auto file_size = std::filesystem::file_size(file_path);
    auto calculated_size = Padding::CalculateSizeForOnz<uint32_t>(file_size);

    if (calculated_size != file->resourceTwoSize) {
        // Debug::Warning("Wrong Padding for %s ", file_path.c_str());
    }

    Debug::Info(hash_changed ? Debug::LevelH_Low : Debug::LevelH_Medium,
        "Processing file: %s (changed: %s)", file_path.c_str(), hash_changed ? "yes" : "no");
}


bool write_dat_build = false;

void ProcessFile_Build(FileEntry* file, std::string file_path,FileEntry* cache_file,uint32_t local_padding = 0)
{
    auto path = std::filesystem::path(file_path).replace_extension(".onz").string();
    if (auto it = GLocalPadding.find(path); it != GLocalPadding.end())
    {
        local_padding = local_padding + it->second;
    }

    //TODO : Add Time Support, first DateCheck then Hash
    auto hash  = CalculateFileHashXXHash(path);
    //Changed
    if (((hash != 0 && hash != cache_file->resourceOneSize) || ignore_hash_system) && std::filesystem::exists(path))
    {
        write_dat_build = true;
        Debug::Info(Debug::LevelH_Low,"File changed, updating: %s with padding %d", file_path.c_str(), local_padding + gGlobalPadding);
        FFile onz_file;
        onz_file.open(path.c_str(), std::ios::in | std::ios::binary);

        if (file->resourceOneSize != 0 && file->resourceTwoSize != 0) {
            struct LZ11Header {
                uint32_t data;
                uint8_t getCompressionType() const { return data & 0xFF; }
                uint32_t getDecompressedSize() const { return (data >> 8) & 0xFFFFFF; }
            };

            auto one_size = onz_file.read<LZ11Header>().getDecompressedSize();
            auto onz_size = onz_file.size();
            file->resourceOneSize = Padding::CalculateSizeForOne<uint32_t>(one_size, onz_size, local_padding + gGlobalPadding);
            file->resourceTwoSize = Padding::CalculateSizeForOnz<uint32_t>(onz_size);
        }
        else
        {
            file->resourceOneSize = 0;
            file->resourceTwoSize = Padding::CalculateSizeForOnz<uint32_t>(onz_file.size());
        }
        onz_file.close();
    }
}

void ProcessEntry_Build(DirectoryEntry* dir, const std::string& dir_path, DirectoryEntry* cache_dir)
{
    if (!dir || !cache_dir) {
        return;
    }

    for (size_t i = 0; i < dir->directories.size(); ++i) {
        const auto& sub_dir = dir->directories[i];
        const std::string sub_path = dir_path + "\\" + sub_dir->name;

        if (i < cache_dir->directories.size()) {
            ProcessEntry_Build(sub_dir.get(), sub_path, cache_dir->directories[i].get());
        }
    }

    for (size_t i = 0; i < dir->files.size(); ++i) {
        const auto& file = dir->files[i];
        const std::string file_path = dir_path + "\\" + file->name;

        if (i < cache_dir->files.size()) {
            ProcessFile_Build(file.get(), file_path, cache_dir->files[i].get());
        }
    }
}

int mode_build(const std::string& dat_path, uint32_t padding_value) {
    auto dat_cache_path = std::filesystem::path(dat_path).replace_extension(".cbin");
    DatFile dat, dat_cache;

    dat.open(dat_path.c_str(), std::ios::in | std::ios::out | std::ios::binary);
    dat_cache.open(dat_cache_path.string().c_str(), std::ios::in | std::ios::out | std::ios::binary);

    dat.ReadFile();
    dat_cache.ReadFile();

    auto root_dir = std::filesystem::path(dat_path).parent_path().string();

    if (path_to_.size() > 0) {
        for (const auto& path : path_to_) {
            auto relative_path = std::filesystem::relative(path, root_dir);
            if (auto file = dat.root->FindFile(relative_path.string()); auto cache_file = dat_cache.root->FindFile(relative_path.string())) {
                ProcessFile_Build(file, path,cache_file,0);
            }
        }
    }
    else {
        ProcessEntry_Build(dat.root.get(), root_dir, dat_cache.root.get());
    }

    dat.jump(0);
    dat_cache.jump(0);

    if (write_dat_build) {
        dat_cache.WriteFile();
        dat.WriteFile();
    }

    dat.close();
    dat_cache.close();
    return 1;
}

std::string trim(const std::string& str) {
    if (str.empty()) return str;

    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) return "";

    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(start, end - start + 1);
}

int mode_pack(std::string path)
{
    DatFile file;
    auto dat_path = std::filesystem::path(path).replace_extension(".dat").string();

    // Read the file content
    std::ifstream inFile(path);
    std::string jsonContent((std::istreambuf_iterator<char>(inFile)),
        std::istreambuf_iterator<char>());

    // Parse as RapidJSON Document
    rapidjson::Document doc;
    doc.Parse(jsonContent.c_str());

    if (doc.HasParseError()) {
        return -1; // Parse error
    }

    // DESERIALIZE WITH THE JSON VALUE
    file.Deserialize(doc); 
   
    file.open(dat_path.c_str(), std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
    file.WriteFile();
    file.close();

    return 0;
}

int main(int argc, char* argv[])
{
#ifdef _DEBUG
    // Simulate command line arguments
    std::vector<std::string> debugArgs = {
         "program.exe",
         "C:\\Users\\bogdanovich\\Downloads\\one_file\\onefiles.json",
         "--pack",
         "--force",
         "--local_padding=\" C:\\Users\\bogdanovich\\Downloads\\one_file\\load_main_game\\stage011\\common_dump.onz,272232\""
         "--local_padding_file=\"\C:\\Users\\bogdanovich\\source\\repos\\unwiished_dat_tool\\scripts\\padding_entries.txt\""
    };

    // Override argc/argv for debugging
    std::vector<char*> debugArgv;
    for (const auto& arg : debugArgs) {
        debugArgv.push_back(const_cast<char*>(arg.c_str()));
    }

    argc = debugArgv.size();
    argv = debugArgv.data();
#endif

    if (argc < 2) {
        Debug::Info("Usage: unleashed_dat_tool <path_to_file> [--build\--unpack\..e.g]");
        Debug::Info("  path_to_file - path to .dat file");
        Debug::Info("  --unpack - .dat to .json");
        Debug::Info("  --pack - .json to .dat");
        Debug::Info("  --build - create dat file from json (not implemented yet)");
        Debug::Info("  --padding=value - add extra padding manually, for example area.onz(for original 1248) and any common_dump.onz(here arround 200000 - 375000)");
        Debug::Info("  --path = \".path_1,path_2 ... .\"(specify files");
        Debug::Info("  --local_padding=\"path1,padding1,path2,padding2,...\" - Add extra padding for specific files, for example area.onz(for original 1248) and any common_dump.onz(should be 200000 - 375000)s");

        Debug::Info("  --force - ignore hash system");

        Debug::Info("example # 1 --build --padding=1024");
        Debug::Info("example # 2 --build --padding=1024 --local_padding=\"path1,padding1,path2,padding2,....\"");
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

        // Remove spaces only from the argument name part (before =)
        size_t equals_pos = arg.find('=');
        if (equals_pos != std::string::npos) {
            // Has value: --flag=value
            std::string flag_part = arg.substr(0, equals_pos);
            std::string value_part = arg.substr(equals_pos + 1);

            // Remove spaces ONLY from the flag part
            flag_part.erase(std::remove(flag_part.begin(), flag_part.end(), ' '), flag_part.end());
            arg = flag_part + "=" + value_part;
        }
        else {
            // No value: just --flag - remove all spaces
            arg.erase(std::remove(arg.begin(), arg.end(), ' '), arg.end());
        }

        // Now check the cleaned argument
        if (arg == "--build") {
            build_mode = true;
            continue;
        }

        if (arg == "--unpack") {
            build_unpack = true;
            continue;
        }

        if (arg == "--read") {
            build_read = true;
            continue;
        }

        if (arg == "--pack") {
            build_pack = true;
            continue;
        }


        if (arg.find("--path=") == 0) {
            std::string pathValue = arg.substr(7); // After "--path="
            // pathValue keeps its spaces for file paths!
            std::stringstream ss(pathValue);
            std::string path;

            while (std::getline(ss, path, ',')) {
                path = trim(path);
                path_to_.push_back(path); // Paths with spaces preserved
            }
            continue;
        }

        // --local_padding=path1,padding1,path2,padding2,...
        if (arg.find("--local_padding=") == 0) {
            std::string pathValue = arg.substr(16); // After "--local_padding="
            
            // Remove surrounding quotes if present
            if (pathValue.size() >= 2 && pathValue.front() == '"' && pathValue.back() == '"') {
                pathValue = pathValue.substr(1, pathValue.size() - 2);
            }

            std::stringstream ss(pathValue);
            std::string path;
            std::string padding_str;

            while (std::getline(ss, path, ',') && std::getline(ss, padding_str, ',')) {
                path = trim(path);
                Debug::Info("Registered padding for %s = %s", path.c_str(), padding_str.c_str());
                GLocalPadding[path] = std::stoi(padding_str); // Paths with spaces preserved
            }
            continue;
        }

        if (arg.find("--local_padding_file=") == 0) {
            std::string filename = arg.substr(21); // After "--local_padding_file="

            // Remove surrounding quotes if present
            if (filename.size() >= 2 && filename.front() == '"' && filename.back() == '"') {
                filename = filename.substr(1, filename.size() - 2);
            }

            // Read from the file
            std::ifstream file(filename);
            if (!file.is_open()) {
                std::cerr << "Error: Could not open padding file: " << filename << std::endl;
                return 1;
            }

            std::string line;
            while (std::getline(file, line)) {
                // Skip empty lines and comments
                if (line.empty() || line[0] == ';') continue;

                std::stringstream ss(line);
                std::string path;
                std::string padding_str;

                // Parse each line as: path,padding_value
                if (std::getline(ss, path, ',') && std::getline(ss, padding_str, ',')) {
                    path = trim(path);
                    padding_str = trim(padding_str);
                    // Remove any trailing commas from padding
                    padding_str.erase(std::remove(padding_str.begin(), padding_str.end(), ','), padding_str.end());

                    if (!path.empty() && !padding_str.empty()) {
                        try {
                            auto p = std::stoi(padding_str);
                            GLocalPadding[path] = p;
                            Debug::Info("Registered padding for %s = ", path, p);
                        }
                        catch (const std::exception& e) {
                            Debug::Error("Something wrong with padding file[%s] check for syntax, %s - %s - %s", padding_str,__FILE__, __LINE__, e.what());
                            std::cerr << "Error parsing padding value: " << padding_str << " for path: " << path << std::endl;
                        }
                    }
                }
            }
            file.close();
            continue;
        }

        if (arg == "--force") {
            ignore_hash_system = true;
            continue;
        }

        if (arg.find("--padding=") == 0) {
            gGlobalPadding = std::stoi(arg.substr(10)); // After "--padding="
            continue;
        }
    }

    if (build_pack)
        mode_pack(input_path);

    if (build_unpack)
        mode_convert(input_path);

    if (build_read)
        mode_read(input_path);

    if (build_mode)
        mode_build(input_path, padding_value);

}