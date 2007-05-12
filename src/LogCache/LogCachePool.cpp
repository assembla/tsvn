#include "StdAfx.h"
#include "LogCachePool.h"
#include "CachedLogInfo.h"

// begin namespace LogCache

namespace LogCache
{

// utility

bool CLogCachePool::FileExists (const std::wstring& filePath)
{
	return GetFileAttributes (filePath.c_str()) != INVALID_FILE_ATTRIBUTES;
}

// construction / destruction
// (Flush() on destruction)

CLogCachePool::CLogCachePool (const CString& cacheFolderPath)
	: cacheFolderPath (cacheFolderPath)
{
}

CLogCachePool::~CLogCachePool()
{
	Flush();
	Clear();
}

// auto-create and return cache for given repository

CCachedLogInfo* CLogCachePool::GetCache (const CString& uuid)
{
	// cache hit?

	TCaches::const_iterator iter = caches.find (uuid);
	if (iter != caches.end())
		return iter->second;

	// load / create

	std::wstring fileName = (LPCTSTR)(cacheFolderPath + _T("\\") + uuid);
	std::auto_ptr<CCachedLogInfo> cache (new CCachedLogInfo (fileName));

	if (FileExists (fileName))
		cache->Load();

	caches[uuid] = cache.get();

	// ready

	return cache.release();
}

// cache management

// write all changes to disk

void CLogCachePool::Flush()
{
	for ( TCaches::iterator iter = caches.begin(), end = caches.end()
		; iter != end
		; ++iter)
	{
		if (iter->second->IsModified())
			iter->second->Save();
	}
}

// minimize memory usage

void CLogCachePool::Clear()
{
	while (!caches.empty())
	{
		CCachedLogInfo* toDelete = caches.begin()->second;
		caches.erase (caches.begin());
		delete toDelete;
	}
}

// end namespace LogCache

}

