#include "StdAfx.h"
#include ".\foldercrawler.h"
#include "SVNStatusCache.h"

CFolderCrawler::CFolderCrawler(void)
{
	m_hWakeEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	m_hTerminationEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	m_hThread = INVALID_HANDLE_VALUE;
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
	ATLASSERT(path.IsDirectory());

	{
		AutoLocker lock(m_critSec);
		m_foldersToUpdate.push_back(path);
	}

	ATLTRACE("Q4Crawl: %ws\n", path.GetWinPath());

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

		// We must have been woken-up
		Sleep(1000);

		for(;;)
		{
			CTSVNPath workingPath;
			{
				AutoLocker lock(m_critSec);
				if(m_foldersToUpdate.empty())
				{
					// Nothing left to do 
					break;
				}
				workingPath = m_foldersToUpdate.front();
				m_foldersToUpdate.pop_front();
			}

			ATLTRACE("Crawling folder: %s\n", workingPath.GetSVNApiPath());

			// Now, we need to visit this folder, to make sure that we know its 'most important' status
			CSVNStatusCache::Instance().RefreshDirectoryStatus(workingPath);

			Sleep(10);
		}
	}
}
