#pragma once

///////////////////////////////////////////////////////////////
// base class includes
///////////////////////////////////////////////////////////////

#include "logiteratorbase.h"

///////////////////////////////////////////////////////////////
// CLogBatchIteratorBase
///////////////////////////////////////////////////////////////

class CLogBatchIteratorBase : public CLogIteratorBase
{
public:

	// container type for <path, revision> pairs

	typedef std::pair<CDictionaryBasedPath, size_t> TPathRevision;
	typedef std::vector<TPathRevision> TPathRevisions;

protected:

	// the paths to log for and their revisions

	TPathRevisions pathRevisions;

	// path list folding utilties

	static size_t MaxRevision (const TPathRevisions& pathRevisions);
	static CDictionaryBasedPath BasePath ( const CCachedLogInfo* cachedLog
										 , const TPathRevisions& pathRevisions);

	// construction 
	// (copy construction & assignment use default methods)

	CLogBatchIteratorBase ( const CCachedLogInfo* cachedLog
						  , const TPathRevisions& startPathRevisions);

	// implement log scanning sub-routines to
	// work on multiple paths

	virtual size_t SkipNARevisions();
	virtual bool PathInRevision() const;

	virtual void ToNextRevision();

public:

	// destruction

	virtual ~CLogBatchIteratorBase(void);
};
