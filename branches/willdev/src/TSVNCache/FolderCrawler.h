#pragma once

#include "TSVNPath.h"

//////////////////////////////////////////////////////////////////////////





class CFolderCrawler
{
public:
	CFolderCrawler(void);
	~CFolderCrawler(void);

public:
	void Initialise();
	void AddDirectoryForUpdate(const CTSVNPath& path);

private:
	static unsigned int __stdcall ThreadEntry(void* pContext);
	void WorkerThread();
	void SetHoldoff();

private:
	CComAutoCriticalSection m_critSec;
	HANDLE m_hThread;
	std::deque<CTSVNPath> m_foldersToUpdate;
	HANDLE m_hTerminationEvent;
	HANDLE m_hWakeEvent;
	LONG m_bCrawlInhibitSet;
	long m_crawlHoldoffReleasesAt;


	friend class CCrawlInhibitor;
};


//////////////////////////////////////////////////////////////////////////


class CCrawlInhibitor
{
private:
	CCrawlInhibitor(); // Not defined
public:
	explicit CCrawlInhibitor(CFolderCrawler* pCrawler)
	{
		m_pCrawler = pCrawler;
		::InterlockedExchange(&m_pCrawler->m_bCrawlInhibitSet, 1);
	}
	~CCrawlInhibitor()
	{
		::InterlockedExchange(&m_pCrawler->m_bCrawlInhibitSet, 0);
		m_pCrawler->SetHoldoff();
	}
private:
	CFolderCrawler* m_pCrawler;
};
