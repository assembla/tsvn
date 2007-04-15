#include "StdAfx.h"
#include "StrictLogBatchIterator.h"

// implement as no-op

bool CStrictLogBatchIterator::HandleCopyAndDelete()
{
	return false;
}

// construction / destruction 

CStrictLogBatchIterator::CStrictLogBatchIterator
	( const CCachedLogInfo* cachedLog
	, const TPathRevisions& startPathRevisions)
	: CLogBatchIteratorBase (cachedLog, startPathRevisions)
{
}

CStrictLogBatchIterator::~CStrictLogBatchIterator(void)
{
}
