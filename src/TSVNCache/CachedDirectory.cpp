#include "StdAfx.h"
#include ".\cacheddirectory.h"
#include "SVNHelpers.h"
#include "SVNStatusCache.h"
#include "SVNStatus.h"
#include <ShlObj.h>

#include <wfext.h>

CCachedDirectory::CCachedDirectory(void)
{
	m_entriesFileTime = 0;
	m_mostImportantStatus = svn_wc_status_none;
}

CCachedDirectory::~CCachedDirectory(void)
{
}

CCachedDirectory::CCachedDirectory(const CTSVNPath& directoryPath)
{
	ATLASSERT(directoryPath.IsDirectory());

	m_directoryPath = directoryPath;
	m_entriesFileTime = 0;
	m_mostImportantStatus = svn_wc_status_none;
}

CStatusCacheEntry CCachedDirectory::GetStatusForMember(const CTSVNPath& path)
{

	int i = sizeof(svn_wc_status_t);


	// In all normal circumstances, we ask for the status of a member of this directory.
	// JUST SOMETIMES, we need to ask for the status of the directory itself...
	// (The SOMETIMES occur if we're in an unversioned folder above the top of a WC)
	ATLASSERT(m_directoryPath.IsEquivalentTo(path.GetContainingDirectory()) || path.IsEquivalentTo(m_directoryPath));

	// Check if the entries file has been changed
	CTSVNPath entriesFilePath(m_directoryPath);
	entriesFilePath.AppendPathString(_T(SVN_WC_ADM_DIR_NAME) _T("\\entries"));
	if(m_entriesFileTime == entriesFilePath.GetLastWriteTime())
	{
		if(m_entriesFileTime == 0)
		{
			// We are a folder which is not in a working copy
			// However, a member folder might be the top of WC
			// so we need to ask them to get their own status
			if(!path.IsDirectory())
			{
				return CStatusCacheEntry();
			}
			else
			{
				// If we're in the special case of a directory being asked for its own status
				// and this directory is unversioned, then we should just return that here
				if(path.IsEquivalentTo(m_directoryPath))
				{
					return CStatusCacheEntry();
				}

				// Ask the main cache to look up the actual directory
				return CSVNStatusCache::Instance().GetDirectorysOwnStatus(path);
			}
		}

		CacheEntryMap::iterator itMap = m_entryCache.find(GetCacheKey(path));
		if(itMap != m_entryCache.end())
		{
			// We've hit the cache - check for timeout
			if(!itMap->second.HasExpired((long)GetTickCount()))
			{
				if(itMap->second.DoesFileTimeMatch(path.GetLastWriteTime()))
				{
					return itMap->second;
				}
				else
				{
					ATLTRACE("Filetime change on file %s\n", path.GetSVNApiPath());
				}
			}
			else
			{
				ATLTRACE("Cache timeout on file %s\n", path.GetSVNApiPath());
			}
		}
	}
	else
	{
		if(m_entriesFileTime != 0)
		{
			ATLTRACE("Entries file change seen for %s\n", path.GetSVNApiPath());
		}
		m_entriesFileTime = entriesFilePath.GetLastWriteTime();
		m_entryCache.clear();
	}

	CSVNStatusCache& mainCache = CSVNStatusCache::Instance();
	SVNPool subPool(mainCache.m_svnHelp.Pool());
	svn_opt_revision_t revision;
	revision.kind = svn_opt_revision_unspecified;

	// We've not got this item in the cache - let's add it
	// We never bother asking SVN for the status of just one file, always for its containing directory

	if(path.IsDirectory() && path.GetFileOrDirectoryName().CompareNoCase(_T(SVN_WC_ADM_DIR_NAME)) == 0)
	{
		// We're being asked for the status of an .SVN directory
		// It's not worth asking for this
		AddEntry(path, NULL);
		return CStatusCacheEntry();
	}

	ATLTRACE("svn_cli_stat for '%s' (req %s)\n", m_directoryPath.GetSVNApiPath(), path.GetSVNApiPath());

	m_mostImportantStatus = svn_wc_status_unversioned;

	svn_error_t* pErr = svn_client_status (
		NULL,
		m_directoryPath.GetSVNApiPath(),
		&revision,
		GetStatusCallback,
		this,
		mainCache.m_bDoRecursiveFetches,		//descend
		TRUE,									//getall
		mainCache.m_bGetRemoteStatus,			//update
		TRUE,									//noignore
		mainCache.m_svnHelp.ClientContext(),
		subPool
		);

	if(pErr)
	{
		// Handle an error
		// The most likely error on a folder is that it's not part of a WC
		// This should have been caught earlier
		ATLASSERT(FALSE);
		return CStatusCacheEntry();
	}

	PushMostImportantStatusUpwards();

	CacheEntryMap::iterator itMap = m_entryCache.find(GetCacheKey(path));
	if(itMap != m_entryCache.end())
	{
		return itMap->second;
	}

	// We've failed to find our target path in the cache
	AddEntry(path, NULL);
	return CStatusCacheEntry();
}

void 
CCachedDirectory::AddEntry(const CTSVNPath& path, const svn_wc_status_t* pSVNStatus)
{
	m_entryCache[GetCacheKey(path)] = CStatusCacheEntry(pSVNStatus,path.GetLastWriteTime());
}


CString 
CCachedDirectory::GetCacheKey(const CTSVNPath& path)
{
	// All we put into the cache as a key is just the end portion of the pathname
	// There's no point storing the path of the containing directory for every item
	return path.GetWinPathString().Mid(m_directoryPath.GetWinPathString().GetLength()).MakeLower();
}

CString 
CCachedDirectory::GetFullPathString(const CString& cacheKey)
{
	return m_directoryPath.GetWinPathString() + _T("\\") + cacheKey;
}

void CCachedDirectory::GetStatusCallback(void *baton, const char *path, svn_wc_status_t *status)
{
	CCachedDirectory* pThis = (CCachedDirectory*)baton;

	CTSVNPath svnPath;

	if(status->entry)
	{
		svnPath.SetFromSVN(path, (status->entry->kind == svn_node_dir));

		if(svnPath.IsDirectory() && svnPath.GetWinPathString().GetLength() != pThis->m_directoryPath.GetWinPathString().GetLength())
		{
			// Add any directory, which is not our 'self' entry, to the list for having its status updated
			CSVNStatusCache::Instance().AddFolderForCrawling(svnPath);
		}
	}
	else
	{
		svnPath.SetFromSVN(path);
	}

	pThis->m_mostImportantStatus = SVNStatus::GetMoreImportant(pThis->m_mostImportantStatus, status->text_status);
	pThis->m_mostImportantStatus = SVNStatus::GetMoreImportant(pThis->m_mostImportantStatus, status->prop_status);

	pThis->AddEntry(svnPath, status);
}

void CCachedDirectory::SetStatusOfContainedDirectory(const CTSVNPath& directoryPath, svn_wc_status_kind newStatus)
{
	ATLASSERT(directoryPath.IsDirectory());

	// Look and see if we know about this entry
	CacheEntryMap::iterator itMap = m_entryCache.find(GetCacheKey(directoryPath));
	if(itMap != m_entryCache.end())
	{
		// We do have this directory - change its status 
		if(itMap->second.ForceStatus(newStatus))
		{
			// The status was changed - we need to tell the shell to change its folder icon
//			SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH | SHCNF_FLUSHNOWAIT, itMap->first.GetWinPath(), NULL);
			SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSHNOWAIT, GetFullPathString(itMap->first), NULL);
		}
	}

	// We also need to make sure that our own 'most important' status is updated to this level
	svn_wc_status_kind newStatusForThisFolder = newStatus;
	CacheEntryMap::iterator itSelfEntry = m_entryCache.end();
	for(itMap = m_entryCache.begin(); itMap != m_entryCache.end(); ++itMap)
	{
		// Ignore our 'self' entry on the way through - it could confuse us with stale state
		if(itMap->first.IsEmpty())
		{
			itSelfEntry = itMap;
		}
		else
		{
			newStatusForThisFolder = SVNStatus::GetMoreImportant(newStatusForThisFolder, itMap->second.GetEffectiveStatus());
		}
	}
	if(m_mostImportantStatus != newStatusForThisFolder)
	{
		m_mostImportantStatus = newStatus;

		// We can also update our own 'self' entry, in case we're asked for that
		if(itSelfEntry != m_entryCache.end())
		{
			itSelfEntry->second.ForceStatus(m_mostImportantStatus);
		}

		// And push this change up to *our* parent
		CSVNStatusCache::Instance().SetDirectoryStatusInParent(m_directoryPath, m_mostImportantStatus);
	}
}

void
CCachedDirectory::PushMostImportantStatusUpwards()
{
	// We now know the 'most important' status for this directory
	// We can push this back to our parent directory, which contains an entry for us
	CSVNStatusCache::Instance().SetDirectoryStatusInParent(m_directoryPath, m_mostImportantStatus);
}