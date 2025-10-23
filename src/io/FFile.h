#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>
#include <queue>
#include "IWRData.h"
#include <map>
#include <unordered_map>
#include <sstream>

struct _Empty_
{

};
class FFile
{
public:
    enum Endian { None, Little, Big };

    FFile() : handle(INVALID_HANDLE_VALUE), endian(Little) {}

    ~FFile() { close(); }

    bool open(const char* path, DWORD desiredAccess, DWORD creationDisposition);

    void close();

    void flush();

    template<typename T> T read(Endian endian = None)
    {
        T value;
        ReadFile(handle, &value, sizeof(T), nullptr, nullptr);
        if (endian == Big || this->endian == Big)
            value = swap(value);
        return value;
    }
    template<typename T> void write(T value)
    {
        if (endian == Big || this->endian == Big)
            value = swap(value);
        WriteFile(handle, &value, sizeof(T), nullptr, nullptr);
    }

    void read_pointer(IWRData data);

    void write_pointer(IWRData data);

 
    template <typename T = _Empty_>
    void write_mark(std::string mark, T value, bool write_value = true)
    {
        // Find existing mark
        auto it = std::find_if(_markers.begin(), _markers.end(),
            [&](const auto& pair) { return pair.first == mark; });

        if (it != _markers.end()) {
            // Update existing mark - add new offset
            it->second.push_back(tell());
        }
        else {
            // Create new mark
            _markers.emplace_back(mark, std::vector<uint64_t>{tell()});
        }

        if (write_value) write<T>(value);
    }

    template <typename T = _Empty_>
    T get_mark(std::string mark)
    {
        auto it = std::find_if(_markers.begin(), _markers.end(),
            [&](const auto& pair) { return pair.first == mark; });

        if (it == _markers.end() || it->second.empty()) {
            // Handle error - mark not found or no offsets
            return T{};
        }

        auto cur = tell();
        auto offset = it->second.front(); // Get first offset
        return offset;
    }

    template <typename T = _Empty_>
    void fill_mark(std::string mark, T value)
    {
        auto it = std::find_if(_markers.begin(), _markers.end(),
            [&](const auto& pair) { return pair.first == mark; });

        if (it == _markers.end() || it->second.empty()) {
            // Handle error - mark not found
            return;
        }

        auto cur = tell();
        auto offset = it->second.front(); // Get first offset
        jump(offset);
        write<T>(value);
        jump(cur);
    }

    std::vector<std::pair<std::string, std::vector<uint64_t>>> get_mark(std::string prefix)
    {
        std::vector<std::pair<std::string, std::vector<uint64_t>>> result;
        for (auto& mark : _markers)
        {
            if (mark.first.compare(0, prefix.length(), prefix) == 0)
                result.push_back(mark);
        }
        return result;
    }

    uint64_t size();
    uint64_t tell();

    void jump(uint64_t to);

    // Specializations for strings
    std::string readString(size_t len);

    std::string readNullString();
  
    void SetEndian(Endian e = Little);

    void writeString(const char* str);

private:
    HANDLE handle;
    Endian endian;
    std::map<IWRData, uint64_t> _pointers;
    std::vector<std::pair<std::string, std::vector<uint64_t>>> _markers;

    template<typename T> T swap(T value)
    {
        if constexpr (!std::is_fundamental_v<T> && !std::is_enum_v<T> && !std::is_pointer_v<T>) {
            return value;
        }
        else if  constexpr (sizeof(T) == 1) {
            return value;
        }
        else if constexpr (sizeof(T) == 2) {
            return _byteswap_ushort(value);
        }
        else if constexpr (sizeof(T) == 4) {
            return _byteswap_ulong(value);
        }
        else if constexpr (sizeof(T) == 8) {
            return _byteswap_uint64(value);
        }
        else {
            uint8_t* bytes = (uint8_t*)&value;
            for (size_t i = 0; i < sizeof(T) / 2; i++)
                std::swap(bytes[i], bytes[sizeof(T) - 1 - i]);
            return value;
        }
    }
};