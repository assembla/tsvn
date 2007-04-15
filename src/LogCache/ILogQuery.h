#pragma once

///////////////////////////////////////////////////////////////
// ILogQuery
///////////////////////////////////////////////////////////////

class ILogReceiver;

class ILogQuery
{
public:

	// query a section from log for multiple paths
	// (special revisions, like "HEAD", supported)

	virtual void Log ( const CTSVNPathList& targets
					 , const SVNRev& peg_revision
					 , const SVNRev& start
					 , const SVNRev& end
					 , int limit
					 , bool strictNodeHistory
					 , ILogReceiver* receiver) = 0;
};
