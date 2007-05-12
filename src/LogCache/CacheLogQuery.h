#pragma once

#include "ILogQuery.h"

class CCachedLogInfo;
class CSVNLogQuery;

class CCacheLogQuery : public ILogQuery
{
private:

	class CLogFiller : private ILogReceiver
	{
	private:

		// cache to use & update

		CCachedLogInfo* cache;

		// connection to the SVN repository

		CSVNLogQuery* svnQuery;

		// prepare a call to svnQuery to fill up a gap in the cache

		revision_t LastMissingRevision ( revision_t firstMissingRevision
									   , revision_t endRevision);

	public:

		void FillLog ( revision_t startRevision
					 , revision_t endRevision
					 , const CDictionaryBasedPath& startPath
					 , int limit
					 , bool strictNodeHistory
					 , ILogReceiver* receiver);
	};

	// we get our cache from here

	CLogCachePool* caches;

	// cache to use & update

	CCachedLogInfo* cache;
	CStringA URL;

	// connection to the SVN repository (may be NULL)

	CSVNLogQuery* svnQuery;

	// 

	revision_t FillLog ( revision_t startRevision
					   , revision_t endRevision
					   , const CDictionaryBasedPath& startPath
					   , int limit
					   , bool strictNodeHistory
					   , ILogReceiver* receiver);

	// fill the receiver's change list buffer 

	static std::auto_ptr<LogChangedPathArray> GetChanges 
		( CRevisionInfoContainer::CChangesIterator& first
		, CRevisionInfoContainer::CChangesIterator& last);

	// crawl the history and forward it to the receiver

	void InternalLog ( revision_t startRevision
					 , revision_t endRevision
					 , const CDictionaryBasedPath& startPath
					 , int limit
					 , bool strictNodeHistory
					 , ILogReceiver* receiver);

	// follow copy history until the startRevision is reached

	CDictionaryBasedPath TranslatePegRevisionPath 
		( revision_t pegRevision
		, revision_t startRevision
		, const CDictionaryBasedPath& startPath);

	// extract the repository-relative path of the URL / file name
	// and open the cache

	CDictionaryBasedPath GetRelativeRepositoryPath (SVNInfoData& info);

	// get UUID & repository-relative path

	SVNInfoData& GetRepositoryInfo ( const CTSVNPath& path
 								   , SVNInfoData& baseInfo
								   , SVNInfoData& headInfo) const;

	// decode special revisions:
	// base / head must be initialized with NO_REVISION
	// and will be used to cache these values.

	revision_t DecodeRevision ( const CTSVNPath& path
				  			  , const SVNRev& revision
							  , SVNInfoData& baseInfo
							  , SVNInfoData& headInfo) const;

	// get the (exactly) one path from targets
	// throw an exception, if there are none or more than one

	CTSVNPath GetPath (const CTSVNPathList& targets) const;

public:

	// construction / destruction

	CCacheLogQuery (CLogCachePool* caches, CSVNLogQuery* svnQuery);
	virtual ~CCacheLogQuery(void);

	// query a section from log for multiple paths
	// (special revisions, like "HEAD", supported)

	virtual void Log ( const CTSVNPathList& targets
					 , const SVNRev& peg_revision
					 , const SVNRev& start
					 , const SVNRev& end
					 , int limit
					 , bool strictNodeHistory
					 , ILogReceiver* receiver);
};
