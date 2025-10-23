#include "BuildReadMode.h"
#include <iostream>
#include <filesystem>


void ProcessEntry(DirectoryEntry& entry, std::string path)
{
    std::filesystem::create_directories(path);

    for (auto& dir : entry.directories)
    {
        auto current_dir_path = path + "\\" + dir.name;
        ProcessEntry(dir, current_dir_path);
    }

    // Process files
    for (auto& file : entry.files)
    {
        auto file_path = path + "\\" + file.name;
        Debug::Info("Extracting file: %s", file_path.c_str());

    }
}

void BuildReadMode::Process(DatFile dat,std::string path)
{
	ProcessEntry(dat.root,path);
}
