#pragma once

///////////////////////////////////////////////////////////////
// forward declarations
///////////////////////////////////////////////////////////////

class CDictionaryBasedPath;

///////////////////////////////////////////////////////////////
// ILogIterator
///////////////////////////////////////////////////////////////

class ILogIterator
{
public:

	// data access

	virtual bool DataIsMissing() const = 0;
	virtual size_t GetRevision() const = 0;
	virtual const CDictionaryBasedPath& GetPath() const = 0;

	// to next / previous revision for our path

	virtual void Advance() = 0;

	// call this after DataIsMissing() and you added new
	// revisions to the cache

	virtual void Retry() = 0;
};
