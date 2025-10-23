#include "DatFile.h"
#include <format>
#include <filesystem>


DatFile::DatFile() : FFile()
{
    root = std::make_shared<DirectoryEntry>();
}

DatFile::~DatFile()
{
}

void DatFile::Serialize(Value& value, Document::AllocatorType& allocator)
{
    value.SetObject();
    value.AddMember("HEAD", rapidjson::StringRef("ones", 4), allocator);
    rapidjson::Value rootValue(rapidjson::kObjectType);
    root->Serialize(rootValue, allocator);
    value.AddMember("root", rootValue, allocator);
}

void DatFile::Deserialize(const rapidjson::Value& value)
{
    if (value.HasMember("root") && value["root"].IsObject())
    {
        root = std::make_shared<DirectoryEntry>();
        root->name = "root";
        root->Deserialize(value["root"]);
    }
}

void DatFile::ReadFile()
{
    auto start = tell();

    auto str = readString(4);
    if (str != "ones")
    {
        Debug::Error("File is Not Valid DAT");
        return;
    }

    Debug::Info("File is Valid DAT");

    auto size = read<uint32_t>();
    jump(0x10);
    SetEndian(FFile::Big);

    root->name = "root";
    root->Read(this);
}

void DatFile::WriteFile()
{
    writeString("ones");
    write_mark("dat_size", -1);

    SetEndian(Big);
    write<uint32_t>(1);
    write<uint32_t>(0);

    root->Write(this);
    auto marks = this->get_marks("#string_");
    for (auto& pair : marks)
    {
        Debug::Info(Debug::LevelH_Medium,14,pair.first.c_str());

        auto _string_ = pair.first.substr(8);
        size_t eof_pos = _string_.find("#EOF#");
        if (eof_pos != std::string::npos) {
            _string_ = _string_.substr(0, eof_pos);
        }

        auto _offset_ = tell();
        writeString(_string_.c_str());
        write<char>(0);
        auto _before_ = tell();

        for (auto& offset : pair.second)
        {
            jump(offset);
            write<uint32_t>(_offset_);
        }
        jump(_before_);
    }
    SetEndian(Little);

    write<char>(0);
    fill_mark<uint32_t>("dat_size", tell());
}

void FileEntry::Read(FFile* reader)
{
    resourceOneSize = reader->read<uint32_t>();
    resourceTwoSize = reader->read<uint32_t>();
}

void FileEntry::Write(FFile* writer)
{
    writer->write<uint32_t>(resourceOneSize);
    writer->write<uint32_t>(resourceTwoSize);
}

void FileEntry::Serialize(rapidjson::Value& value, rapidjson::Document::AllocatorType& allocator)
{
        value.SetObject();
        value.AddMember("name", rapidjson::Value(name.c_str(), allocator), allocator);
        value.AddMember("resourceOneSize", resourceOneSize, allocator);
        value.AddMember("resourceTwoSize", resourceTwoSize, allocator);
}

void FileEntry::Deserialize(const rapidjson::Value& value)
{
    name = value["name"].GetString();
    resourceOneSize = value["resourceOneSize"].GetInt();
    resourceTwoSize = value["resourceTwoSize"].GetInt();
}

DirectoryEntry::DirectoryEntry()
{
    directoryNames = std::vector<std::string>();
    fileNames = std::vector<std::string>();
    directories = std::vector<std::shared_ptr<DirectoryEntry>>();
    files = std::vector<std::shared_ptr<FileEntry>>();
}

//DirectoryEntry
void DirectoryEntry::Read(FFile* reader)
{
    auto start = reader->tell();
    directoryCount = reader->read<uint32_t>();
    fileCount = reader->read<uint32_t>();
    directoriesNameOffset = reader->read<uint32_t>();
    filesNameOffset = reader->read<uint32_t>();
    directoriesDataOffset = reader->read<uint32_t>();
    filesDataOffset = reader->read<uint32_t>();
    Debug::Info(Debug::LevelH_Medium,8,"DirectoryEntry: offset : %x, name - %s, dir - %d, file - %d", start, name.c_str(), directoryCount, fileCount);

    // Read directory names
    for (uint32_t i = 0; i < directoryCount; i++)
    {
        reader->jump(directoriesNameOffset + i * 4);
        uint32_t stringOffset = reader->read<uint32_t>();

        reader->jump(stringOffset);
        directoryNames.push_back(reader->readNullString());
    }

    // Read file names
    for (uint32_t i = 0; i < fileCount; i++)
    {
        reader->jump(filesNameOffset + i * 4);
        uint32_t stringOffset = reader->read<uint32_t>();

        reader->jump(stringOffset);
        fileNames.push_back(reader->readNullString());
    }

    // Read directory data
    reader->jump(directoriesDataOffset);
    for (uint32_t i = 0; i < directoryCount; i++)
    {
        reader->jump(directoriesDataOffset + i * 0x18);

        auto dir = std::make_shared<DirectoryEntry>();
        dir->name = directoryNames[i];
        dir->Read(reader);
        directories.push_back(dir);
    }

    reader->jump(filesDataOffset);
    for (uint32_t i = 0; i < fileCount; i++)
    {
        auto file = std::make_shared<FileEntry>();
        file->Read(reader);
        file->name = fileNames[i];
        files.push_back(file);
    }
}

// Helper function to convert pointer to string
std::string ptr_to_string(const void* ptr) {
    std::ostringstream oss;
    oss << ptr;
    return oss.str();
}

//Dirs
void WriteDir(DirectoryEntry& dir)
{


  
}
void DirectoryEntry::Write(FFile* writer)
{
    bool is_root = (this->name == "root");

    if (is_root)
    {
        // Write directory header
        writer->write<uint32_t>(directories.size());
        writer->write<uint32_t>(files.size());
        writer->write_mark<uint32_t>("DirectoryEntry_Names_" + ptr_to_string(this), 0);
        writer->write_mark<uint32_t>("Files_Names_" + ptr_to_string(this), 0);
        writer->write_mark<uint32_t>("DirectoryEntry_Table_" + ptr_to_string(this), 0);
        writer->write_mark<uint32_t>("Files_Table_" + ptr_to_string(this), 0);

    }

    // Write directory names
    if (directories.size() > 0) {
        writer->fill_mark<uint32_t>("DirectoryEntry_Names_" + ptr_to_string(this), writer->tell());
        for (auto& dir : directories) {
            writer->write_mark<uint32_t>("#string_" + dir->name + + "#EOF#" + ptr_to_string(&dir) , -1, true);
        }
    }

    // Write file names
    if (files.size() > 0) {
        writer->fill_mark<uint32_t>("Files_Names_" + ptr_to_string(this), writer->tell());
        for (auto& file : files) {
            writer->write_mark<uint32_t>("#string_" + file->name + +"#EOF#" + ptr_to_string(&file), -1, true);
        }
    }

    // Write directory table headers (all subdirectories at current level)
    if (directories.size() > 0) {
        writer->fill_mark<uint32_t>("DirectoryEntry_Table_" + ptr_to_string(this), writer->tell());
        for (auto& dir : directories) {
            // Write header for each subdirectory
            writer->write<uint32_t>(dir->directories.size());
            writer->write<uint32_t>(dir->files.size());
            writer->write_mark<uint32_t>("DirectoryEntry_Names_" + ptr_to_string(dir.get()), 0);
            writer->write_mark<uint32_t>("Files_Names_" + ptr_to_string(dir.get()), 0);
            writer->write_mark<uint32_t>("DirectoryEntry_Table_" + ptr_to_string(dir.get()), 0);
            writer->write_mark<uint32_t>("Files_Table_" + ptr_to_string(dir.get()), 0);
        }
    }

    // Write files for current directory
    if (files.size() > 0) {
        writer->fill_mark<uint32_t>("Files_Table_" + ptr_to_string(this), writer->tell());
        for (auto& file : files) {
            file->Write(writer);
        }
    }

    // Recursively write all subdirectories
    for (auto& dir : directories) {
        dir->Write(writer);  // Recursive call
    }
}

void DirectoryEntry::Serialize(rapidjson::Value& value, rapidjson::Document::AllocatorType& allocator)
{
    value.SetObject();

    value.AddMember("name", rapidjson::Value(name.c_str(), allocator), allocator);

    // Create array for files
    rapidjson::Value filesArray(rapidjson::kArrayType);
    for (auto& file : files) {
        rapidjson::Value fileObj(rapidjson::kObjectType);
        file->Serialize(fileObj, allocator);
        filesArray.PushBack(fileObj, allocator);
    }
    value.AddMember("Files", filesArray, allocator);

    rapidjson::Value dirsArray(rapidjson::kArrayType);
    for (auto& dir : directories) {
        rapidjson::Value dirObj(rapidjson::kObjectType);
        dir->Serialize(dirObj, allocator);
        dirsArray.PushBack(dirObj, allocator);
    }
    value.AddMember("Directories", dirsArray, allocator);
}

void DirectoryEntry::Deserialize(const rapidjson::Value& value)
{
    // Get name
    if (value.HasMember("name") && value["name"].IsString()) {
        name = value["name"].GetString();
    }

    // Get files array
    if (value.HasMember("Files") && value["Files"].IsArray()) {
        const rapidjson::Value& filesArray = value["Files"];
        files.clear(); // Clear existing files

        for (const auto& fileValue : filesArray.GetArray())
        {
            auto file = std::make_shared< FileEntry>();
            file->Deserialize(fileValue);
            files.push_back(file);
        }
    }

    if (value.HasMember("Directories") && value["Directories"].IsArray()) {
        const rapidjson::Value& dirsArray = value["Directories"];
        directories.clear(); // Clear existing directories

        for (const auto& dirValue : dirsArray.GetArray()) 
        {
                auto subdir = std::make_shared< DirectoryEntry>();
                subdir->Deserialize(dirValue);
                directories.push_back(subdir);
        }
    }
}

DirectoryEntry* DirectoryEntry::FindDirectory(const std::string& path)
{
    DirectoryEntry* current = this;

    for (const auto& component : std::filesystem::path(path)) {
        std::string name = component.string();
        if (name.empty() || name == ".") continue;

        bool found = false;
        for (auto& dir : current->directories) {
            if (dir->name == name) 
            {
                current = dir.get();
                found = true;
                break;
            }
        }
        if (!found) return current;
    }
    return current;
}

FileEntry* DirectoryEntry::FindFile(const std::string& path)
{
    auto dir = this;
    auto fs_path = std::filesystem::path(path);

    if (fs_path.root_directory().filename() != dir->name)
        dir = FindDirectory(path);

    if (!dir) return nullptr;

    auto target_name = fs_path.stem().string();
    for (auto& file : dir->files) {
        if (std::filesystem::path(file->name).stem().string() == target_name)
            return file.get();
    }
    return nullptr;
}
