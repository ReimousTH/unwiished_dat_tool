#pragma once

#include <fstream>
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <memory>
#include <io/IWRData.h>

struct _Empty_ {};

class FFile
{
public:
    enum Endian { None, Little, Big };

    FFile();
    ~FFile();

    bool open(const std::string& path, std::ios_base::openmode mode);
    void close();
    void flush();

    template<typename T> T read(Endian endian = None);
    template<typename T> void write(T value);

    void read_pointer(IWRData* data);
    void write_pointer(IWRData* data);

    template <typename T = _Empty_>
    void write_mark(std::string mark, T value, bool write_value = true);

    template <typename T = _Empty_>
    T get_mark(std::string mark);

    template <typename T = _Empty_>
    void fill_mark(std::string mark, T value);

    std::vector<std::pair<std::string, std::vector<uint64_t>>> get_marks(const std::string& prefix);

    uint64_t size();
    uint64_t tell();
    void jump(uint64_t to);

    // Specializations for strings
    std::string readString(size_t len);
    std::string readNullString();
    void SetEndian(Endian e = Little);
    void writeString(const std::string& str);

private:
    std::fstream file;
    Endian endian;
    std::map<IWRData*, uint64_t> _pointers;
    std::vector<std::pair<std::string, std::vector<uint64_t>>> _markers;

    template<typename T> T swap(T value);
};

#include "FFile.inl"