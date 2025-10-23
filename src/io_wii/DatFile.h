#pragma once
#include <io/FFile.h>
#include <system/Debug.h>
#include <io/ISerializable.h>
#include <io/IWRData.h>
#include <memory>

class FFile;
class IWRData;

struct FileEntry : IWRData, ISerializable
{
    uint32_t resourceOneSize;
    uint32_t resourceTwoSize;
    std::string name;

    virtual void Read(FFile* reader) override;
    virtual void Write(FFile* writer) override;
    virtual void Serialize(rapidjson::Value& value, rapidjson::Document::AllocatorType& allocator) override;
    virtual void Deserialize(const rapidjson::Value& value) override;
};

struct DirectoryEntry : IWRData, ISerializable
{
    uint32_t directoryCount;
    uint32_t fileCount;
    uint32_t directoriesNameOffset;
    uint32_t filesNameOffset;
    uint32_t directoriesDataOffset;
    uint32_t filesDataOffset;

    std::string name;
    std::vector<std::string> directoryNames;
    std::vector<std::string> fileNames;
    std::vector<std::shared_ptr<DirectoryEntry>> directories;
    std::vector<std::shared_ptr<FileEntry>> files;

    DirectoryEntry();

    virtual void Read(FFile* reader) override;

    virtual void Write(FFile* writer) override;

    virtual bool operator<(const IWRData& other) const override
    {
        return this < &other;
    }

    virtual void Serialize(rapidjson::Value& value, rapidjson::Document::AllocatorType& allocator) override;

    virtual void Deserialize(const rapidjson::Value& value);

    DirectoryEntry* FindDirectory(const std::string& path);
    FileEntry* FindFile(const std::string& path);



};


class DatFile:public FFile, ISerializable
{
public:
	DatFile();
	~DatFile();
    std::shared_ptr<DirectoryEntry> root;
    

	void ReadFile();
    void WriteFile();

    virtual void Serialize(Value& value, Document::AllocatorType& allocator);
    virtual void Deserialize(const Value& value);
};

