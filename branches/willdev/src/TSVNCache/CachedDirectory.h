#pragma once

#include "StatusCacheEntry.h"
#include "TSVNPath.h"

class CCachedDirectory
{
public:

	CCachedDirectory();
//	CCachedDirectory(const svn_wc_status_t* pSVNStatus, __int64 lastWriteTime);
	CCachedDirectory(const CTSVNPath& directoryPath);
	~CCachedDirectory(void);
	bool IsVersioned() const;
	CStatusCacheEntry GetStatusForMember(const CTSVNPath& path);

	svn_wc_status_kind GetMostImportantStatus();

	void SetStatusOfContainedDirectory(const CTSVNPath& directoryPath, svn_wc_status_kind newStatus);
	void PushMostImportantStatusUpwards();

private:
	static void GetStatusCallback(void *baton, const char *path, svn_wc_status_t *status);
	void AddEntry(const CTSVNPath& path, const svn_wc_status_t* pSVNStatus);
	CString GetCacheKey(const CTSVNPath& path);
	CString GetFullPathString(const CString& cacheKey);

private:
	// The cache of files and directories within this directory
	typedef std::map<CString, CStatusCacheEntry> CacheEntryMap; 
	CacheEntryMap m_entryCache; 

	// The timestamp of the .SVN\entries file.  For an unversioned directory, this will be zero
	__int64 m_entriesFileTime;

	CTSVNPath	m_directoryPath;

	svn_wc_status_kind m_mostImportantStatus;

//	friend class CSVNStatusCache;		
};

