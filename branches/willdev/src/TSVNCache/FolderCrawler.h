#pragma once

#include "TSVNPath.h"

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

private:
	CComAutoCriticalSection m_critSec;
	HANDLE m_hThread;
	std::deque<CTSVNPath> m_foldersToUpdate;
	HANDLE m_hTerminationEvent;
	HANDLE m_hWakeEvent;


};
