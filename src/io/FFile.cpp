#include "FFile.h"

bool FFile::open(const char* path, DWORD desiredAccess, DWORD creationDisposition)
{
    handle = CreateFileA(
        path,
        desiredAccess,
        FILE_SHARE_READ,
        nullptr,
        creationDisposition,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    return handle != INVALID_HANDLE_VALUE;
}

void FFile::close()
{
    if (handle != INVALID_HANDLE_VALUE)
    {
        flush();
        CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
    }
}
void FFile::flush()
{
    if (handle != INVALID_HANDLE_VALUE) {
        FlushFileBuffers(handle);
    }
}

void FFile::read_pointer(IWRData data)
{
    auto ptr = read<uint32_t>();
    auto pos = tell();
    jump(ptr);
    data.Read(this);
    jump(pos);
}

void FFile::write_pointer(IWRData data)
{
    _pointers[data] = tell();
    write<uint32_t>(0);
}

uint64_t FFile::tell()
{
    LARGE_INTEGER pos;
    pos.QuadPart = 0;
    SetFilePointerEx(handle, pos, &pos, FILE_CURRENT);
    return pos.QuadPart;
}

uint64_t FFile::size()
{
    LARGE_INTEGER fileSize;
    if (GetFileSizeEx(handle, &fileSize)) {
        return fileSize.QuadPart;
    }
    return 0; // or handle error
}

void FFile::jump(uint64_t to)
{
    LARGE_INTEGER pos;
    pos.QuadPart = to;
    SetFilePointerEx(handle, pos, nullptr, FILE_BEGIN);
}

std::string FFile::readString(size_t len)
{
    auto buf = new char[len + 1];
    ReadFile(handle, buf, len, nullptr, nullptr);
    buf[len] = '\0';
    auto result = std::string(buf);
    delete[] buf;
    return result;
}

std::string FFile::readNullString()
{
    std::string buffer;

    char c = 0;
    do
    {
        c = read<char>();
        buffer.push_back(c);
    } while (c != 0);

    buffer.pop_back();
    return buffer;
}

void FFile::SetEndian(Endian e)
{
    endian = e;
}

void FFile::writeString(const char* str)
{
    WriteFile(handle, str, strlen(str), nullptr, nullptr);
}
