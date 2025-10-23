#pragma once
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/error/en.h>

using namespace rapidjson;

class ISerializable {
public:
    virtual ~ISerializable();
    virtual void Serialize(Value& value, Document::AllocatorType& allocator);
    virtual void Deserialize(const Value& value);
};