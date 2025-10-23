#include "FFile.h"
#include <algorithm>

FFile::FFile() : endian(Endian::Little) {}

FFile::~FFile() {
    close();
}

bool FFile::open(const std::string& path, std::ios_base::openmode mode) {
    file.open(path, mode);
    return file.is_open();
}

void FFile::close() {
    if (file.is_open()) {
        flush();
        file.close();
    }
}

void FFile::flush() {
    if (file.is_open()) {
        file.flush();
    }
}

void FFile::read_pointer(IWRData* data) {
    auto ptr = read<uint32_t>();
    auto pos = tell();
    jump(ptr);
    data->Read(this);
    jump(pos);
}

void FFile::write_pointer(IWRData* data) {
    _pointers[data] = tell();
    write<uint32_t>(0);
}

uint64_t FFile::tell() {
    return static_cast<uint64_t>(file.tellg());
}

uint64_t FFile::size() {
    auto currentPos = tell();
    file.seekg(0, std::ios::end);
    auto fileSize = tell();
    jump(currentPos);
    return fileSize;
}

void FFile::jump(uint64_t to) {
    file.seekg(static_cast<std::streampos>(to));
    file.seekp(static_cast<std::streampos>(to));
}

std::string FFile::readString(size_t len) {
    std::vector<char> buffer(len + 1);
    file.read(buffer.data(), len);
    buffer[len] = '\0';
    return std::string(buffer.data());
}

std::string FFile::readNullString() {
    std::string buffer;
    char c = 0;

    do {
        file.get(c);
        if (c != 0 && file.good()) {
            buffer.push_back(c);
        }
    } while (c != 0 && file.good());

    return buffer;
}

void FFile::writeString(const std::string& str) {
    file.write(str.c_str(), str.length());
}

void FFile::SetEndian(Endian e) {
    endian = e;
}

std::vector<std::pair<std::string, std::vector<uint64_t>>> FFile::get_marks(const std::string& prefix) {
    std::vector<std::pair<std::string, std::vector<uint64_t>>> result;
    for (const auto& mark : _markers) {
        if (mark.first.compare(0, prefix.length(), prefix) == 0) {
            result.push_back(mark);
        }
    }
    return result;
}

