#include "StdAfx.h"
#include "CacheLogQuery.h"
#include "CachedLogInfo.h"

// fill the receiver's change list buffer 

std::auto_ptr<LogChangedPathArray>
CCacheLogQuery::GetChanges ( CRevisionInfoContainer::CChangesIterator& first
						   , CRevisionInfoContainer::CChangesIterator& last)
{
	std::auto_ptr<LogChangedPathArray> result (new LogChangedPathArray);

	for (; first != last; ++first)
	{
		// find the item in the hash

		std::auto_ptr<LogChangedPath> changedPath (new LogChangedPath);

		// extract the path name

		std::string path = first.GetPath().GetPath();
		changedPath->sPath = SVN::MakeUIUrlOrPath (path.c_str());

		// decode the action

		CRevisionInfoContainer::TChangeAction action = first.GetAction();
		changedPath->action = (DWORD)action / 4;

		// decode copy-from info

		if (   first.GetFromPath().IsValid()
			&& (first.GetFromRevision() != NO_REVISION))
		{
			std::string path = first.GetFromPath().GetPath();

			changedPath->lCopyFromRev = first.GetFromRevision();
			changedPath->sCopyFromPath 
				= SVN::MakeUIUrlOrPath (path.c_str());
		}
		else
		{
			changedPath->lCopyFromRev = 0;
		}

		arChangedPaths->Add (changedPath.release());
	} 

	return result;
}

// crawl the history and forward it to the receiver

void CCacheLogQuery::InternalLog ( revision_t startRevision
								 , revision_t endRevision
								 , const CDictionaryBasedPath& startPath
								 , int limit
								 , bool strictNodeHistory
								 , ILogReceiver* receiver)
{
	// create the right iterator

	std::auto_ptr<ILogIterator> iterator
		= strictNodeHistory
		? new CStrictLogInterator (cache, startRevision, startPath)
		: new CCopyFollowingLogIterator (cache, startRevision, startPath);

	// find first suitable entry or cache gap

	iterator->Retry();

	// report starts at endRevision or earlier revisions

	revision_t lastReported = endRevision+1;

	// crawl & update the cache, report entries found

	while ((iterator->GetRevision() >= endRevision) && !iterator->EndOfPath())
	{
		if (iterator->DataIsMissing())
		{
			// our cache is incomplete -> fill it.
			// Report entries immediately to the receiver 
			// (as to allow the user to cancel this action).

			lastReported = FillLog ( iterator->GetRevision()
								   , startRevision
								   , iterator->GetPath()
								   , limit
								   , strictNodeHistory
								   , receiver);

			iterator->Retry();
		}
		else
		{
			// found an entry. Report it if not already done.

			revision_t revision = iterator->GetRevision();
			if (revision < lastReported)
			{
				index_t logIndex = cache->GetRevisions (revision);
				const CRevisionInfoContainer& logInfo = cache->GetLogInfo();

				std::auto_ptr<LogChangedPathArray> changes
					= GetChanges ( logInfo->GetChangesBegin (logIndex)
								 , logInfo->GetChangesEnd (logIndex));

				receiver->ReceiveLog ( *changes.get())
									 , revision
									 , CUnicodeUtils::GetUnicode 
										   (logInfo->GetAuthor (index).c_str())
									 , logInfo->GetTimeStamp (index)
									 , CUnicodeUtils::GetUnicode 
									       (logInfo->GetComment (index).c_str()));
			}

			// enough?

			if ((limit != 0) && (--limit == 0))
				return;
		}
	}
}

// follow copy history until the startRevision is reached

CDictionaryBasedPath CCacheLogQuery::TranslatePegRevisionPath 
	( revision_t pegRevision
	, revision_t startRevision
	, const CDictionaryBasedPath& startPath)
{
	CCopyFollowingLogIterator iterator (cache, startRevision, startPath);

	while ((iterator.GetRevision() > pegRevision) && !iterator.EndOfPath())
	{
		if (iterator.DataIsMissing())
			FillLog ( iterator.GetRevision()
					, startRevision
					, iterator.GetPath()
					, 0
					, false
					, NULL);

		iterator.Advance();
	}

	return iterator.GetPath();
}

// extract the repository-relative path of the URL / file name
// and open the cache

CDictionaryBasedPath CCacheLogQuery::GetRelativeRepositoryPath (SVNInfoData& info)
{
	// load cache

	cache = caches->GetCache (info.reposUUID);
	URL = CUnicodeUtils::GetUTF8 (info.reposRoot);

	// get path object

	CStringA relPath = CUnicodeUtils::GetUTF8 (info.url).Mid (URL.GetLength());

	return CDictionaryBasedPath (cache->paths, (const char*)relPath);
}

// get UUID & repository-relative path

SVNInfoData& CCacheLogQuery::GetRepositoryInfo ( const CTSVNPath& path
 								               , SVNInfoData& baseInfo
									           , SVNInfoData& headInfo) const
{
	// already known?

	if (baseInfo.IsValid())
		return baseInfo;

	if (headInfo.IsValid())
		return headInfo;

	// look it up

	if (path.IsUrl())
	{
		headInfo = *SVNInfo().GetFirstFileInfo (path, SVNRev(), _T("HEAD"));
		return headInfo;
	}
	else
	{
		baseInfo = *SVNInfo().GetFirstFileInfo (path, SVNRev(), SVNRev());
		return baseInfo;
	}
}

// decode special revisions:
// base / head must be initialized with NO_REVISION
// and will be used to cache these values.

revision_t CCacheLogQuery::DecodeRevision ( const CTSVNPath& path
			  							  , const SVNRev& revision
										  , SVNInfoData& baseInfo
										  , SVNInfoData& headInfo) const
{
	if (!revision.IsValid())
		throw SVNError ( SVN_ERR_CLIENT_BAD_REVISION
					   , "Invalid revision passed to Log().");

	// efficently decode standard cases: revNum, HEAD, BASE/WORKING

	switch (revision.GetKind())
	{
	case svn_opt_revision_number:
		return static_cast<LONG>(revision);

	case svn_opt_svn_opt_revision_head:
		if (!headInfo.IsValid())
			headInfo = *SVNInfo().GetFirstFileInfo (path, SVNRev(), revision);

		return static_cast<LONG>(headInfo.rev);

	case svn_opt_revision_base:
	case svn_opt_svn_opt_revision_working:
		if (!baseInfo.IsValid())
			baseInfo = *SVNInfo().GetFirstFileInfo (path, SVNRev(), SVNRev());

		return static_cast<LONG>(baseInfo.rev);
	}

	// more unusual cases:

	SVNInfo infoProvider;
	const SVNInfoData* info 
		= infoProvider.GetFirstFileInfo (path, SVNRev(), revision);

	return static_cast<LONG>(info.rev);
}

// get the (exactly) one path from targets
// throw an exception, if there are none or more than one

CTSVNPath CCacheLogQuery::GetPath (const CTSVNPathList& targets) const
{
	if (targets.GetCount() != 1)
		throw SVNError ( SVN_ERR_INCORRECT_PARAMS
					   , "Must specify exactly one path to get the log from.");

	return targets.operator [0];
}

// construction / destruction

CCacheLogQuery::CCacheLogQuery(void)
{
}

CCacheLogQuery::~CCacheLogQuery(void)
{
}

// query a section from log for multiple paths
// (special revisions, like "HEAD", supported)

void CCacheLogQuery::Log ( const CTSVNPathList& targets
						 , const SVNRev& peg_revision
						 , const SVNRev& start
						 , const SVNRev& end
						 , int limit
						 , bool strictNodeHistory
						 , ILogReceiver* receiver)
{
	// the path to log for

	CTSVNPath path = GetPath (targets);

	// decode revisions

	SVNInfoData baseInfo;
	SVNInfoData headInfo;

	revision_t startRevision = DecodeRevision (path, start, baseInfo, headInfo);
	revision_t endRevision = DecodeRevision (path, end, baseInfo, headInfo);
	revision_t pegRevision = DecodeRevision (path, peg_revision, baseInfo, headInfo);

	// order revisions

	if (endRevision > startRevision)
		std::swap (endRevision, startRevision);

	if (pegRevision < startRevision)
		pegRevision = startRevision;

	// load cache and find path to start from

	SVNInfoData& info = GetRepositoryInfo (path, baseInfo, headInfo);

	CDictionaryBasedPath startPath 
		= TranslatePegRevisionPath ( pegRevision
								   , startRevision
								   , GetRelativeRepositoryPath (path, info));

	// do it 

	InternalLog ( startRevision
				, endRevision
				, startPath
				, limit
				, strictNodeHistory
				, receiver);
}
