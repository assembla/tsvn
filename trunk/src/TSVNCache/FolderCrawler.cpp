// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005 - Will Dean

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#include "StdAfx.h"
#include ".\foldercrawler.h"
#include "SVNStatusCache.h"

CFolderCrawler::CFolderCrawler(void)
{
	m_hWakeEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	m_hTerminationEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	m_hThread = INVALID_HANDLE_VALUE;
	m_bCrawlInhibitSet = 0;
	m_crawlHoldoffReleasesAt = (long)GetTickCount();
}

CFolderCrawler::~CFolderCrawler(void)
{
	SetEvent(m_hTerminationEvent);
	if(WaitForSingleObject(m_hThread, 5000) != WAIT_OBJECT_0)
	{
		ATLTRACE("Error terminating crawler thread\n");
	}
	CloseHandle(m_hThread);
	CloseHandle(m_hTerminationEvent);
	CloseHandle(m_hWakeEvent);
}

void 
CFolderCrawler::Initialise()
{
	// Don't call Initalise more than once
	ATLASSERT(m_hThread == INVALID_HANDLE_VALUE);

	AutoLocker lock(m_critSec);

	unsigned int threadId;
	m_hThread = (HANDLE)_beginthreadex(NULL,0,ThreadEntry,this,0,&threadId);
	SetThreadPriority(m_hThread, THREAD_PRIORITY_IDLE);
}

void 
CFolderCrawler::AddDirectoryForUpdate(const CTSVNPath& path)
{
	if (!PathFileExists(path.GetWinPath()))
	{
		// the path doesn't exist anymore (e.g. directory deleted, renamed, moved)
		CSVNStatusCache::Instance().RemoveCacheForPath(path);
		return;
	}
	
	ATLASSERT(path.IsDirectory());

	{
		AutoLocker lock(m_critSec);
		m_foldersToUpdate.push_back(path);
	}

	ATLTRACE("Q4Crawl: %ws\n", path.GetWinPath());

	m_bItemsAddedSinceLastCrawl = true;
	SetEvent(m_hWakeEvent);
}


unsigned int CFolderCrawler::ThreadEntry(void* pContext)
{
	((CFolderCrawler*)pContext)->WorkerThread();
	return 0;
}

void CFolderCrawler::WorkerThread()
{
	HANDLE hWaitHandles[2];
	hWaitHandles[0] = m_hTerminationEvent;	
	hWaitHandles[1] = m_hWakeEvent;

	for(;;)
	{

		DWORD waitResult = WaitForMultipleObjects(sizeof(hWaitHandles)/sizeof(hWaitHandles[0]), hWaitHandles, FALSE, INFINITE);
		if(waitResult == WAIT_OBJECT_0 || waitResult == WAIT_ABANDONED)
		{
			// Termination event
			break;
		}

		// If we get here, we've been woken up by something being added to the queue.
		// However, it's important that we don't do our crawling while
		// the shell is still asking for items
		// 

		for(;;)
		{
			CTSVNPath workingPath;

			if(m_bCrawlInhibitSet)
			{
				// We're in crawl hold-off 
				ATLTRACE("Crawl hold-off\n");
				Sleep(50);
				continue;
			}

			{
				AutoLocker lock(m_critSec);
				if(m_foldersToUpdate.empty())
				{
					// Nothing left to do 
					break;
				}

				if(((long)GetTickCount() - m_crawlHoldoffReleasesAt) < 0)
				{
					lock.Unlock();
					Sleep(50);
					continue;
				}

				if(m_bItemsAddedSinceLastCrawl)
				{
					// The queue has changed - it's worth sorting and de-duping
					std::sort(m_foldersToUpdate.begin(), m_foldersToUpdate.end());
					m_foldersToUpdate.erase(std::unique(m_foldersToUpdate.begin(), m_foldersToUpdate.end(), &CTSVNPath::PredLeftEquivalentToRight), m_foldersToUpdate.end());
					m_bItemsAddedSinceLastCrawl = false;
				}

				workingPath = m_foldersToUpdate.front();
				m_foldersToUpdate.pop_front();
			}

			ATLTRACE("Crawling folder: %ws\n", workingPath.GetWinPath());

			// Now, we need to visit this folder, to make sure that we know its 'most important' status
			CSVNStatusCache::Instance().GetDirectoryCacheEntry(workingPath).RefreshStatus();

			Sleep(10);
		}
	}
	_endthread();
}

void CFolderCrawler::SetHoldoff()
{
	AutoLocker lock(m_critSec);
	m_crawlHoldoffReleasesAt = (long)GetTickCount() + 100;
}
