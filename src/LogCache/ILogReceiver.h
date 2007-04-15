#pragma once

///////////////////////////////////////////////////////////////
// ILogReceiver
///////////////////////////////////////////////////////////////

class ILogReceiver
{
	// call-back for every revision found
	// (called at most once per revision)

	virtual void ReceiveLog ( const LogChangedPathArray& changes
							, svn_revnum_t rev, 
							, const CString& author
							, const apr_time_t& timeStamp
							, const CString& message) = 0;
};
