#pragma once

///////////////////////////////////////////////////////////////
// data structures to accomodate the change list
///////////////////////////////////////////////////////////////

struct LogChangedPath
{
	CString sPath;
	CString sCopyFromPath;
	svn_revnum_t lCopyFromRev;
	DWORD action;
	CString GetAction() const;
};

enum
{
	LOGACTIONS_MODIFIED	= 0x00000001,
	LOGACTIONS_REPLACED	= 0x00000002,
	LOGACTIONS_ADDED	= 0x00000004,
	LOGACTIONS_DELETED	= 0x00000008
};

typedef CArray<LogChangedPath*, LogChangedPath*> LogChangedPathArray;

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
