#pragma once

#include "TSVNPath.h"
#include "SVNHelpers.h"
#include "StatusCacheEntry.h"
#include "CachedDirectory.h"
#include "FolderCrawler.h"

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////


class CSVNStatusCache
{
private:
	
private:
	CSVNStatusCache(void);
	~CSVNStatusCache(void);

public:
	static CSVNStatusCache& Instance();
	static void Create();
	static void Destroy();

public:
	void Clear();
	CStatusCacheEntry GetStatusForPath(const CTSVNPath& path);
	void Dump();
	void EnableRecursiveFetch(bool bEnable);
	void EnableRemoteStatus(bool bEnable);

	// Get the status for a directory by asking the directory, rather 
	// than the usual method of asking its parent
	CStatusCacheEntry GetDirectorysOwnStatus(const CTSVNPath& path);

	void RefreshDirectoryStatus(const CTSVNPath& path);

	// Set the status of a entry directory within a parent
	void SetDirectoryStatusInParent(const CTSVNPath& childDirectory, svn_wc_status_kind status);

	void AddFolderForCrawling(const CTSVNPath& path);

private:
	CCachedDirectory& GetDirectoryCacheEntry(const CTSVNPath& path);

private:
	typedef std::map<CTSVNPath, CCachedDirectory> CachedDirMap; 
	CachedDirMap m_directoryCache; 
	CComAutoCriticalSection m_critSec;
	SVNHelper m_svnHelp;
	bool m_bDoRecursiveFetches;
	bool m_bGetRemoteStatus;

	static CSVNStatusCache* m_pInstance;

	CFolderCrawler m_folderCrawler;

	CTSVNPath m_mostRecentPath;
	CStatusCacheEntry m_mostRecentStatus;
	long m_mostRecentExpiresAt;

	friend class CCachedDirectory;  // Needed for access to the SVN helpers
};
