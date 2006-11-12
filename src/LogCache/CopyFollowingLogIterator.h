#pragma once
#include "logiteratorbase.h"

class CCopyFollowingLogIterator :
	public CLogIteratorBase
{
private:

	// we may need this for paths that have no log entry
	// (e.g. while following a copy history)

	std::string relPath;

public:

	CCopyFollowingLogIterator(void);
	virtual ~CCopyFollowingLogIterator(void);
};
