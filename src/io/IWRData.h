#pragma once

class FFile;

//Reader/Writer Interface
class IWRData
{
public:
	virtual void Read(FFile* reader);
	
	virtual void Write(FFile* writer);

	virtual bool operator<(const IWRData& other) const
	{
		return this < &other;
	}


};

