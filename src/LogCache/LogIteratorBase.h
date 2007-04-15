#pragma once

///////////////////////////////////////////////////////////////
// base class and member includes
///////////////////////////////////////////////////////////////

#include "ILogIterator.h"

#include "PathDictionary.h"
#include "CachedLogInfo.h"

///////////////////////////////////////////////////////////////
// CLogIteratorBase
///////////////////////////////////////////////////////////////

class CLogIteratorBase
{
protected:

	// data source

	const CCachedLogInfo* logInfo;

	// just enough to describe our position
	// and what we are looking for

	size_t revision;
	CDictionaryBasedPath path;

	// construction 
	// (copy construction & assignment use default methods)

	CLogIteratorBase ( const CCachedLogInfo* cachedLog
					 , size_t startRevision
					 , const CDictionaryBasedPath& startPath);

	// implement copy-history following strategy

	virtual bool HandleCopyAndDelete() = 0;

	// do we have data for that revision?

	bool InternalDataIsMissing() const;

	// comparison methods

	static bool PathsIntersect 
		( const CDictionaryBasedPath& lhsPath
		, const CDictionaryBasedPath& rhsPath);
	static bool PathInRevision 
		( const CRevisionInfoContainer::CChangesIterator& first
		, const CRevisionInfoContainer::CChangesIterator& last
		, const CDictionaryBasedPath& path);

	virtual bool PathInRevision() const;

	// utilities for efficient HandleCopyAndDelete() implementation

	// any HandleCopyAndDelete() entries in that revision?

	static bool ContainsCopyOrDelete 
		( const CRevisionInfoContainer::CChangesIterator& first
		, const CRevisionInfoContainer::CChangesIterator& last);

	// return true, searchPath / searchRevision have been modified

	static bool InternalHandleCopyAndDelete 
		( const CRevisionInfoContainer::CChangesIterator& first
		, const CRevisionInfoContainer::CChangesIterator& last
		, const CDictionaryBasedPath& revisionRootPath
		, CDictionaryBasedPath& searchPath
		, size_t& searchRevision);

	// log scanning sub-routines

	virtual size_t SkipNARevisions();
	virtual void ToNextRevision();

	// log scanning

	virtual void InternalAdvance();

public:

	// destruction

	virtual ~CLogIteratorBase(void);

	// implement ILogIterator

	virtual bool DataIsMissing() const;
	virtual size_t GetRevision() const;
	virtual const CDictionaryBasedPath& GetPath() const;

	virtual void Advance();

	virtual void Retry();
};

///////////////////////////////////////////////////////////////
// do we have data for that revision?
///////////////////////////////////////////////////////////////

inline bool CLogIteratorBase::InternalDataIsMissing() const
{
	return logInfo->GetRevisions()[revision] == -1;
}

///////////////////////////////////////////////////////////////
// implement ILogIterator
///////////////////////////////////////////////////////////////

inline size_t CLogIteratorBase::GetRevision() const
{
	return revision;
}

inline const CDictionaryBasedPath& CLogIteratorBase::GetPath() const
{
	return path;
}

