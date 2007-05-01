#pragma once

///////////////////////////////////////////////////////////////
// base class includes
///////////////////////////////////////////////////////////////

#include "logiteratorbase.h"

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

///////////////////////////////////////////////////////////////
// CLogBatchIteratorBase
///////////////////////////////////////////////////////////////

class CLogBatchIteratorBase : public CLogIteratorBase
{
public:

	// container type for <path, revision> pairs

	typedef std::pair<CDictionaryBasedPath, revision_t> TPathRevision;
	typedef std::vector<TPathRevision> TPathRevisions;

protected:

	// the paths to log for and their revisions

	TPathRevisions pathRevisions;

	// path list folding utilties

	static revision_t MaxRevision (const TPathRevisions& pathRevisions);
	static CDictionaryBasedPath BasePath ( const CCachedLogInfo* cachedLog
										 , const TPathRevisions& pathRevisions);

	// construction 
	// (copy construction & assignment use default methods)

	CLogBatchIteratorBase ( const CCachedLogInfo* cachedLog
						  , const TPathRevisions& startPathRevisions);

	// implement log scanning sub-routines to
	// work on multiple paths

	virtual revision_t SkipNARevisions();
	virtual bool PathInRevision() const;

	virtual void ToNextRevision();

public:

	// destruction

	virtual ~CLogBatchIteratorBase(void);
};

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

