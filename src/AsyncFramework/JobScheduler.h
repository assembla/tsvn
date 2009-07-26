/***************************************************************************
 *   Copyright (C) 2009 by Stefan Fuhrmann                                 *
 *   stefanfuhrmann@alice-dsl.de                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#pragma once

#include "CriticalSection.h"
#include "WaitableEvent.h"

class IJob;
class CThread;

class CJobScheduler
{
private:

    /// access sync

    mutable CCriticalSection mutex;

    /// jobs

    typedef std::pair<IJob*, bool> TJob;

    /// low-overhead job queue class

    class CQueue
    {
    private:

        TJob* data;
        TJob* first;
        TJob* last;
        TJob* end;

        /// size management

        void Grow (size_t newSize);
        void AutoGrow();

    public:

        /// construction / destruction

        CQueue()
            : data (NULL)
            , first (NULL)
            , last (NULL)
            , end (NULL)
        {
            Grow (1024);
        }
        ~CQueue()
        {
            delete[] data;
        }

        /// queue I/O

        void push (const TJob& job)
        {
            *last = job;
            if (++last == end)
                AutoGrow();
        }
        const TJob& front() const
        {
            return *first;
        }
        void pop()
        {
            ++first;
        }

        /// size info

        bool empty() const
        {
            return first == last;
        }
        size_t size() const
        {
            return last - first;
        }
        size_t capacity() const
        {
            return end - data;
        }
    };

    CQueue queue;

    /// per-thread info

    struct SThreadInfo
    {
        CJobScheduler* owner;
        CThread* thread;
    };

    /// global thread pool

    class CThreadPool
    {
    private:

        /// list of idle shared threads

        std::vector<SThreadInfo*> pool;

        /// maximum number of entries in \ref pool

        size_t allocCount;
        mutable size_t maxCount;

        /// access sync. object

        CCriticalSection mutex;

        /// schedulers that would like to have more threads left

        std::vector<CJobScheduler*> starving;

        /// no public construction

        CThreadPool();

        /// remove one entry from \ref starving container.
        /// Return NULL, if container was empty

        CJobScheduler* SelectStarving();

        /// no copy support

        CThreadPool(const CThreadPool& rhs);
        CThreadPool& operator=(const CThreadPool& rhs);

    public:

        /// release threads

        ~CThreadPool();

        /// Meyer's singleton

        static CThreadPool& GetInstance();

        /// pool interface

        SThreadInfo* TryAlloc();
        void Release (SThreadInfo* thread);

        /// set max. number of concurrent threads

        void SetThreadCount (size_t count);
        size_t GetThreadCount();

        /// manage starving schedulers 
        /// (must be notified as soon as there is an idle thread)

        /// entry will be auto-removed upon notification

        void AddStarving (CJobScheduler* scheduler);

        /// must be called before destroying the \ref scheduler. 
        /// No-op, if not in \ref starved list.

        void RemoveStarving (CJobScheduler* scheduler);
    };

    /// our thread pool

    struct SThreads
    {
        std::vector<SThreadInfo*> running;
        std::vector<SThreadInfo*> suspended;

        size_t runningCount;
        size_t suspendedCount;

        size_t fromShared;
        size_t maxFromShared;

        /// suspendedCount + maxFromShared - fromShared
        size_t unusedCount;

        /// if set, we registered at shared pool as "starved"
        bool starved;
    };

    SThreads threads;

    /// this will be signalled when the last job is finished

    CWaitableEvent empty;

    /// number of threads in \ref WaitForSomeJobs

    volatile LONG waitingThreads;

    /// this will be signalled when a thread gets suspended

    CWaitableEvent threadIsIdle;

    /// worker thread function

    static void ThreadFunc (void* arg);

    /// job execution helpers

    TJob AssignJob (SThreadInfo* info);
    void RetireJob (const TJob& job);

    /// try to get a thread from the shared pool.
    /// register as "starving" if that failed

    bool AllocateSharedThread();

public:

    /// Create & remove threads.

    CJobScheduler (size_t threadCount, size_t sharedThreads);
    ~CJobScheduler(void);

    /// Meyer's singleton

    static CJobScheduler* GetDefault();

    /// job management:
    /// add new job

    void Schedule (IJob* job, bool transferOwnership);

    /// notification that a new thread may be available
    /// (used internally)

    void ThreadAvailable();

    /// wait for all current and follu-up jobs to terminate

    void WaitForEmptyQueue();

    /// Wait for some jobs to be finished.
    /// This function may return immediately if there are
    /// too many threads waiting in this function.

    void WaitForSomeJobs();

    /// set max. number of concurrent threads

    static void SetSharedThreadCount (size_t count);
    static size_t GetSharedThreadCount();

    static void UseAllCPUs();
    static size_t GetHWThreadCount();
};
