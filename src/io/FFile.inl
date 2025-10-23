template<typename T>
inline T FFile::read(Endian endian)
{
    T value;
    file.read(reinterpret_cast<char*>(&value), sizeof(T));
    
    Endian effectiveEndian = (endian != None) ? endian : this->endian;
    if (effectiveEndian == Big) {
        value = swap(value);
    }
    return value;
}

template<typename T>
inline void FFile::write(T value)
{
    if (endian == Big) {
        value = swap(value);
    }
    file.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template<typename T>
inline void FFile::write_mark(std::string mark, T value, bool write_value)
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

template<typename T>
inline T FFile::get_mark(std::string mark)
{
    auto it = std::find_if(_markers.begin(), _markers.end(),
        [&](const auto& pair) { return pair.first == mark; });

    if (it == _markers.end() || it->second.empty()) {
        // Handle error - mark not found or no offsets
        return T{};
    }

    auto offset = it->second.front(); // Get first offset
    if constexpr (std::is_same_v<T, _Empty_>) {
        return T{};
    } else {
        return static_cast<T>(offset);
    }
}

template<typename T>
inline void FFile::fill_mark(std::string mark, T value)
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

template<typename T>
inline T FFile::swap(T value)
{
    if constexpr (!std::is_fundamental_v<T> && !std::is_enum_v<T> && !std::is_pointer_v<T>) {
        return value;
    }
    else if constexpr (sizeof(T) == 1) {
        return value;
    }
    else {
#if __cplusplus >= 202002L && __has_include(<bit>)
#include <bit>
        return std::byteswap(value);
#else
        // Use compiler built-ins as fallback
#if defined(__GNUC__) || defined(__clang__)
        if constexpr (sizeof(T) == 2) return __builtin_bswap16(value);
        else if constexpr (sizeof(T) == 4) return __builtin_bswap32(value);
        else if constexpr (sizeof(T) == 8) return __builtin_bswap64(value);
#elif defined(_MSC_VER)
        if constexpr (sizeof(T) == 2) return _byteswap_ushort(value);
        else if constexpr (sizeof(T) == 4) return _byteswap_ulong(value);
        else if constexpr (sizeof(T) == 8) return _byteswap_uint64(value);
#else
    // Manual fallback for other sizes
        union {
            T value;
            uint8_t bytes[sizeof(T)];
        } source, dest;
        source.value = value;
        for (size_t i = 0; i < sizeof(T); i++) {
            dest.bytes[i] = source.bytes[sizeof(T) - i - 1];
        }
        return dest.value;
#endif
#endif
    }
}