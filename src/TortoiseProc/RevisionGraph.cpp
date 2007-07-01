// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "StdAfx.h"
#include "resource.h"
#include "client.h"
#include "UnicodeUtils.h"
#include "registry.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "SVN.h"
#include "TSVNPath.h"
#include ".\revisiongraph.h"
#include "SVNError.h"
#include "CachedLogInfo.h"
#include "RevisionIndex.h"
#include "CopyFollowingLogIterator.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void CSearchPathTree::DeLink()
{
	assert (parent);

	if (previous)
		previous->next = next;
	if (next)
		next->previous = previous;

	if (parent->firstChild == this)
		parent->firstChild = next;
	if (parent->lastChild == this)
		parent->lastChild = previous;
}

void CSearchPathTree::Link (CSearchPathTree* newParent)
{
	assert (parent == NULL);
	assert (newParent != NULL);

	parent = newParent;

	previous = parent->lastChild;

	if (parent->firstChild == NULL)
		parent->firstChild = this;
	else
		lastChild->next = this;

	parent->lastChild = this;
}

CSearchPathTree::CSearchPathTree (const CPathDictionary* dictionary)
	: path (dictionary, std::string())
	, startRevision (NO_REVISION)
	, parent (NULL)
	, firstChild (NULL)
	, lastChild (NULL)
	, previous (NULL)
	, next (NULL)
{
}

CSearchPathTree::CSearchPathTree ( const CDictionaryBasedTempPath& path
								 , revision_t startrev
								 , CSearchPathTree* parent)
	: path (path)
	, startRevision (startrev)
	, parent (NULL)
	, firstChild (NULL)
	, lastChild (NULL)
	, previous (NULL)
	, next (NULL)
{
	Link (parent);
}

CSearchPathTree::~CSearchPathTree()
{
	while (firstChild != NULL)
		delete firstChild;

	if (parent)
		DeLink();
}

void CSearchPathTree::Insert ( const CDictionaryBasedTempPath& path
							 , revision_t startrev)
{
	assert (startrev != NO_REVISION);

	// exact match (will happen on root node only)?

	if (this->path == path)
	{
		startRevision = startrev;
		return;
	}

	// (partly or fully) overlap with an existing child?

	for (CSearchPathTree* child = firstChild; child != NULL; child = child->next)
	{
		CDictionaryBasedTempPath commonPath = child->path.GetCommonRoot (path);

		if (commonPath != this->path)
		{
			if (child->path == path)
			{
				// there is already a node for the exact same path
				// -> use it, if unused so far; append a new node otherwise

				if (child->startRevision == NO_REVISION)
					child->startRevision = startrev;
				else
					new CSearchPathTree (path, startrev, this);
			}
			else
			{
				if (child->path == commonPath)
				{
					// the path is a (true) sub-node of the child

					child->Insert (path, startrev);
				}
				else
				{
					// there is a common base path with this child
					// Note: no other child can have that.
					// ->factor out and insert new sub-child

					CSearchPathTree* commonParent 
						= new CSearchPathTree (commonPath, revision_t(NO_REVISION), this);

					child->DeLink();
					child->Link (commonParent);

					new CSearchPathTree (path, startrev, commonParent);
				}
			}

			return;
		}
	}

	// no overlap with any existing node
	// -> create a new child

	new CSearchPathTree (path, startrev, this);
}

bool CSearchPathTree::IsEmpty() const
{
	return (startRevision == NO_REVISION) && (firstChild == NULL);
}

void CSearchPathTree::Remove()
{
	startRevision = revision_t (NO_REVISION);

	CSearchPathTree* node = this;
	while (node->IsEmpty() && (node->parent != NULL))
	{
		CSearchPathTree* temp = node;
		node = node->parent;

		delete temp;
	}
}

CRevisionGraph::CRevisionGraph(void) : m_bCancelled(FALSE)
	, m_FilterMinRev(-1)
	, m_FilterMaxRev(-1)
{
	memset (&m_ctx, 0, sizeof (m_ctx));
	parentpool = svn_pool_create(NULL);

	Err = svn_config_ensure(NULL, parentpool);
	pool = svn_pool_create (parentpool);
	// set up the configuration
	if (Err == 0)
		Err = svn_config_get_config (&(m_ctx.config), g_pConfigDir, pool);

	if (Err != 0)
	{
		::MessageBox(NULL, this->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		svn_error_clear(Err);
		svn_pool_destroy (pool);
		svn_pool_destroy (parentpool);
		exit(-1);
	}

	// set up authentication
	m_prompt.Init(pool, &m_ctx);

	m_ctx.cancel_func = cancel;
	m_ctx.cancel_baton = this;

	//set up the SVN_SSH param
	CString tsvn_ssh = CRegString(_T("Software\\TortoiseSVN\\SSH"));
	if (tsvn_ssh.IsEmpty())
		tsvn_ssh = CPathUtils::GetAppDirectory() + _T("TortoisePlink.exe");
	tsvn_ssh.Replace('\\', '/');
	if (!tsvn_ssh.IsEmpty())
	{
		svn_config_t * cfg = (svn_config_t *)apr_hash_get (m_ctx.config, SVN_CONFIG_CATEGORY_CONFIG,
			APR_HASH_KEY_STRING);
		svn_config_set(cfg, SVN_CONFIG_SECTION_TUNNELS, "ssh", CUnicodeUtils::GetUTF8(tsvn_ssh));
	}
}

CRevisionGraph::~CRevisionGraph(void)
{
	svn_error_clear(Err);
	svn_pool_destroy (parentpool);
	m_arEntryPtrs.RemoveAll();
}

bool CRevisionGraph::SetFilter(svn_revnum_t minrev, svn_revnum_t maxrev, const CString& sPathFilter)
{
	m_FilterMinRev = minrev;
	m_FilterMaxRev = maxrev;
	m_filterpaths.clear();
	// the filtered paths are separated by an '*' char, since that char is illegal in paths and urls
	if (!sPathFilter.IsEmpty())
	{
		int pos = sPathFilter.Find('*');
		if (pos)
		{
			CString sTemp = sPathFilter;
			while (pos>=0)
			{
				m_filterpaths.insert((LPCWSTR)sTemp.Left(pos));
				sTemp = sTemp.Mid(pos+1);
				pos = sTemp.Find('*');
			}
			m_filterpaths.insert((LPCWSTR)sTemp);
		}
		else
			m_filterpaths.insert((LPCWSTR)sPathFilter);
	}
	return true;
}

BOOL CRevisionGraph::ProgressCallback(CString /*text*/, CString /*text2*/, DWORD /*done*/, DWORD /*total*/) {return TRUE;}

svn_error_t* CRevisionGraph::cancel(void *baton)
{
	CRevisionGraph * me = (CRevisionGraph *)baton;
	if (me->m_bCancelled)
	{
		CString temp;
		temp.LoadString(IDS_SVN_USERCANCELLED);
		return svn_error_create(SVN_ERR_CANCELLED, NULL, CUnicodeUtils::GetUTF8(temp));
	}
	return SVN_NO_ERROR;
}

// implement ILogReceiver

void CRevisionGraph::ReceiveLog ( LogChangedPathArray*
								, svn_revnum_t rev
								, const CString&
								, const apr_time_t&
								, const CString&)
{
	// update internal data

	if (m_lHeadRevision < rev)
		m_lHeadRevision = rev;

	// update progress bar and check for user pressing "Cancel" somewhere

	static DWORD lastProgressCall = 0;
	if (lastProgressCall < GetTickCount() - 200)
	{
		lastProgressCall = GetTickCount();

		CString temp, temp2;
		temp.LoadString(IDS_REVGRAPH_PROGGETREVS);
		temp2.Format(IDS_REVGRAPH_PROGCURRENTREV, rev);
		if (!ProgressCallback (temp, temp2, m_lHeadRevision - rev, m_lHeadRevision))
		{
			m_bCancelled = TRUE;
			throw SVNError (cancel (this));
		}
	}
}

BOOL CRevisionGraph::FetchRevisionData(CString path)
{
	// set some text on the progress dialog, before we wait
	// for the log operation to start
	CString temp;
	temp.LoadString(IDS_REVGRAPH_PROGGETREVS);
	ProgressCallback(temp, _T(""), 0, 1);

	// prepare the path for Subversion
	SVN::preparePath(path);
	CStringA url = CUnicodeUtils::GetUTF8(path);

	svn_error_clear(Err);
	// convert a working copy path into an URL if necessary
	if (!svn_path_is_url(url))
	{
		//not an url, so get the URL from the working copy path first
		svn_wc_adm_access_t *adm_access;          
		const svn_wc_entry_t *entry;  
		const char * canontarget = svn_path_canonicalize(url, pool);
#pragma warning(push)
#pragma warning(disable: 4127)	// conditional expression is constant
			Err = svn_wc_adm_probe_open2 (&adm_access, NULL, canontarget,
				FALSE, 0, pool);
			if (Err) return FALSE;
			Err =  svn_wc_entry (&entry, canontarget, adm_access, FALSE, pool);
			if (Err) return FALSE;
			Err = svn_wc_adm_close (adm_access);
			if (Err) return FALSE;
#pragma warning(pop)

			url = entry ? entry->url : "";
	}
	url = CPathUtils::PathEscape(url);

	// we have to get the log from the repository root
	CTSVNPath urlpath;
	urlpath.SetFromSVN(url);

	SVN svn;
	m_sRepoRoot = svn.GetRepositoryRoot(urlpath);
	url = m_sRepoRoot;
	urlpath.SetFromSVN(url);

	if (m_sRepoRoot.IsEmpty())
	{
		Err = svn_error_dup(svn.Err);
		return FALSE;
	}

	m_lHeadRevision = -1;
	try
	{
		CRegStdWORD useLogCache (_T("Software\\TortoiseSVN\\UseLogCache"), TRUE);
		CLogCachePool* caches = useLogCache != FALSE 
							  ? svn.GetLogCachePool() 
							  : NULL;

		svnQuery.reset (new CSVNLogQuery (&m_ctx, pool));
		query.reset (new CCacheLogQuery (caches, svnQuery.get()));

		query->Log ( CTSVNPathList (urlpath)
				   , SVNRev(SVNRev::REV_HEAD)
				   , SVNRev(SVNRev::REV_HEAD)
				   , SVNRev(0)
				   , 0
				   , false
				   , this);
	}
	catch (SVNError& e)
	{
		Err = svn_error_create (e.GetCode(), NULL, e.GetMessage());
		return FALSE;
	}

	return TRUE;
}

BOOL CRevisionGraph::AnalyzeRevisionData(CString path, bool bShowAll /* = false */, bool bArrangeByPath /* = false */)
{
	svn_error_clear(Err);

	m_arEntryPtrs.RemoveAll();
	m_maxurllength = 0;
	m_maxurl.Empty();
	m_numRevisions = 0;
	m_maxlevel = 0;

	SVN::preparePath(path);
	CStringA url = CUnicodeUtils::GetUTF8(path);

	// convert a working copy path into an URL if necessary
	if (!svn_path_is_url(url))
	{
		//not an url, so get the URL from the working copy path first
		svn_wc_adm_access_t *adm_access;          
		const svn_wc_entry_t *entry;  
		const char * canontarget = svn_path_canonicalize(url, pool);
#pragma warning(push)
#pragma warning(disable: 4127)	// conditional expression is constant
		Err = svn_wc_adm_probe_open2 (&adm_access, NULL, canontarget,
			FALSE, 0, pool);
		if (Err) return FALSE;
		Err =  svn_wc_entry (&entry, canontarget, adm_access, FALSE, pool);
		if (Err) return FALSE;
		Err = svn_wc_adm_close (adm_access);
		if (Err) return FALSE;
#pragma warning(pop)

		url = entry ? entry->url : "";
	}

	url = CPathUtils::PathUnescape(url);
	url = url.Mid(CPathUtils::PathUnescape(m_sRepoRoot).GetLength());

	BuildForwardCopies();
	
	// in case our path was renamed and had a different name in the past,
	// we have to find out that name now, because we will analyze the data
	// from lower to higher revisions

	const CCachedLogInfo* cache = query->GetCache();
	const CPathDictionary* paths = &cache->GetLogInfo().GetPaths();
	CDictionaryBasedTempPath startPath (paths, (const char*)url);

	CCopyFollowingLogIterator iterator (cache, m_lHeadRevision, startPath);
	iterator.Retry();
	revision_t initialrev = m_lHeadRevision;

	while (   (iterator.GetRevision() > 0) 
		   && !iterator.EndOfPath()
		   && !iterator.DataIsMissing())
	{
		initialrev = iterator.GetRevision();
		iterator.Advance();
	}

	startPath = iterator.GetPath();
	AnalyzeRevisions (startPath, initialrev, bShowAll);

	return Cleanup(bArrangeByPath);
}

void CRevisionGraph::BuildForwardCopies()
{
	// iterate through all revisions and fill copyToRelation:
	// for every copy-from info found, add an entry

	const CCachedLogInfo* cache = query->GetCache();
	const CRevisionIndex& revisions = cache->GetRevisions();
	const CRevisionInfoContainer& revisionInfo = cache->GetLogInfo();

	// for all revisions ...

	for ( revision_t revision = revisions.GetFirstRevision()
		, last = revisions.GetLastRevision()
		; revision < last
		; ++revision)
	{
		// ... in the cache ...

		index_t index = revisions[revision];
		if (index != NO_INDEX)
		{
			// ... examine all changes ...

			for ( CRevisionInfoContainer::CChangesIterator 
					iter = revisionInfo.GetChangesBegin (index)
				, end = revisionInfo.GetChangesEnd (index)
				; iter != end
				; ++iter)
			{
				// ... and if it has a copy-from info ...

				if (iter->HasFromPath())
				{
					// ... add it to the list

					SCopyTo copyTo;
					copyTo.fromRevision = iter->GetFromRevision();
					copyTo.fromPathIndex = iter->GetFromPathID();
					copyTo.toRevision = revision;
					copyTo.toPathIndex = iter->GetPathID();

					copyToRelation.push_back (copyTo);
				}
			}
		}
	}

	// sort container by source revision and path

	std::sort (copyToRelation.begin(), copyToRelation.end());
}

void CRevisionGraph::AnalyzeRevisions ( const CDictionaryBasedTempPath& path
									  , revision_t startrev
									  , bool bShowAll)
{
	const CCachedLogInfo* cache = query->GetCache();
	const CRevisionIndex& revisions = cache->GetRevisions();
	const CRevisionInfoContainer& revisionInfo = cache->GetLogInfo();

	// initialize the paths we have to search for

	std::auto_ptr<CSearchPathTree> searchTree 
		(new CSearchPathTree (&revisionInfo.GetPaths()));
	searchTree->Insert (path, startrev);

	// collect nodes to draw ... revision by revision

	for (revision_t revision = startrev; revision <= m_lHeadRevision; ++revision)
	{
		index_t index = revisions[revision];
		if (index == NO_INDEX)
			continue;

		CDictionaryBasedPath basePath = revisionInfo.GetRootPath (index);

		CSearchPathTree* searchNode = searchTree.get();
		while (searchNode != NULL)
		{
			if (basePath.IsSameOrParentOf (searchNode->GetPath().GetBasePath()))
			{
				// maybe a hit -> match all changes against the whole sub-tree

				AnalyzeRevisions ( revision
								 , revisionInfo.GetChangesBegin (index)
								 , revisionInfo.GetChangesEnd (index)
								 , searchNode
								 , bShowAll);
			}
			else
			{
				if (   (searchNode->GetFirstChild() != NULL)
					&& searchNode->GetPath().IsSameOrParentOf (basePath))
				{
					// the sub-nodes may be a match

					searchNode = searchNode->GetFirstChild();
				}
				else
				{
					// this sub-tree is no match
					// -> to the next node

					while (   (searchNode->GetNext() == NULL)
						   && (searchNode->GetParent() != NULL))
						searchNode = searchNode->GetParent();

					searchNode = searchNode->GetNext();
				}
			}
		}
	}
}

void CRevisionGraph::AnalyzeRevisions ( revision_t revision
									  , const CDictionaryBasedTempPath& path
									  , CRevisionInfoContainer::CChangesIterator first
									  , CRevisionInfoContainer::CChangesIterator last
									  , CSearchPathTree* rootNode
									  , CSearchPathTree* startNode
									  , std::vector<SCopyTo>::const_iterator firstCopy
									  , std::vector<SCopyTo>::const_iterator lastCopy
									  , bool bShowAll
									  , std::vector<CSearchPathTree*>& toRemove)
{
	// cover the whole sub-tree

	CSearchPathTree* searchNode = startNode;
	do
	{
		// is this search path active?

		if (   (searchNode->GetStartRevision() != NO_REVISION)
			&& (searchNode->GetStartRevision() <= revision))
		{
			const CDictionaryBasedTempPath& path = searchNode->GetPath();

			// looking for the closes change that affected the path

			CRevisionInfoContainer::CChangesIterator lastMatch = last; 
			for ( CRevisionInfoContainer::CChangesIterator iter = first
				; iter != last
				; ++iter)
			{
				if (   iter->GetPath().IsSameOrParentOf (path.GetBasePath())
					&& (   (iter->GetAction() != CRevisionInfoContainer::ACTION_CHANGED)
						|| bShowAll))
				{
					lastMatch = iter;
				}
			}

			// has the path actually been touched?

			if (lastMatch != last)
			{
				CRevisionEntry::Action action (lastMatch->GetAction());
				m_arEntryPtrs.Add (new CRevisionEntry ( searchNode->GetPath()
													  , revision
													  , action));

				if (action == CRevisionEntry::deleted)
					toRemove.push_back (searchNode);
			}

			// add new paths (due to copy operations)

			for ( std::vector<SCopyTo>::const_iterator iter = firstCopy
				; iter != lastCopy
				; ++iter)
			{
				if (searchNode->GetPath().IsSameOrChildOf (iter->fromPathIndex))
				{
					// add & schedule the new search path

					CDictionaryBasedTempPath targetPath 
						= path.ReplaceParent ( CDictionaryBasedPath ( path.GetDictionary()
																    , iter->fromPathIndex)
										     , CDictionaryBasedPath ( path.GetDictionary()
																    , iter->toPathIndex));

					rootNode->Insert (targetPath, iter->fromRevision);

					// add a revision entry for the source, if not yet available

					if (lastMatch == last)
					{
						m_arEntryPtrs.Add (new CRevisionEntry ( path
															  , revision
															  , CRevisionEntry::source));
					}
				}
			}
		}

		// select next node

		if (searchNode->GetFirstChild() != NULL)
		{
			searchNode = searchNode->GetFirstChild();
		}
		else
		{
			while (    (searchNode->GetNext() == NULL)
					&& (searchNode == startNode))
				searchNode == searchNode->GetParent();

			if (searchNode != startNode)
				searchNode = searchNode->GetNext();
		}
	}
	while (searchNode != startNode);
}

bool CRevisionGraph::Cleanup(bool bArrangeByPath)
{
	// step two and a half: rearrange the nodes by path if requested
	if (bArrangeByPath)
	{
		// arraning the nodes by path:
		// each URL gets its own column, as long as there are more than just
		// one node of that URL.

		struct view
		{
			int count;
			int level;
		};
		std::map<std::wstring, view> pathmap;

		// go through all nodes, and count the number of nodes for each URL
		// don't assign a valid level for those yet
		for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
		{
			CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs.GetAt(i);
			if (reventry->url)
			{
				std::wstring surl(reventry->url);
				std::map<std::wstring, view>::iterator findit = pathmap.lower_bound(surl);
				if (findit == pathmap.end() || findit->first != surl)
				{
					findit = pathmap.insert(findit, std::make_pair(surl, view()));
					findit->second.count = 0;
					findit->second.level = 0;
				}
				// that URL already is in the map, so just increase its count
				findit->second.count++;
			}
		}

		// since the map is ordered alphabetically by URL, we assign each URL
		// a level according to its order. This makes sure that e.g. all tags
		// get on the same level, because they will all be ordered after each
		// other in the map.
		int proplevel=1;
		bool bLastWasCountEqualOne = false;
		for (std::map<std::wstring,view>::iterator it = pathmap.begin(); it != pathmap.end(); ++it)
		{
			if (it->second.count > 1)
			{
				if (bLastWasCountEqualOne)
					++proplevel;
				bLastWasCountEqualOne = false;
				it->second.level = proplevel++;
			}
			else
			{
				it->second.level = proplevel;
				bLastWasCountEqualOne = true;
			}
		}

		// ordering the nodes alphabetically by URL doesn't look that good
		// in the graph, because 'branch' comes before 'tags' and only then
		// comes 'trunk'.
		// So we reorder the nodes again, but this time by revisions an URL
		// appears first. That way the graph looks a lot nicer.
		
		std::map<int, int> levelmap;	// Assigns each alphabetical level a real level
		std::map<std::wstring, view>::iterator lev = pathmap.begin();
		int reallevel = 1;
		for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
		{
			CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs.GetAt(i);
			if ((lev = pathmap.find((LPCWSTR)reventry->url)) != pathmap.end())
			{
				std::map<int, int>::iterator levelit = levelmap.lower_bound(lev->second.level);
				if (levelit == levelmap.end() || levelit->first != lev->second.level)
				{
					levelit = levelmap.insert(levelit, std::make_pair(lev->second.level, reallevel));
					++reallevel;
				}
				reventry->level = levelit->second;
			}
			else
				ATLASSERT(FALSE);
		}
	}

	// step four: connect entries with the same name and the same level
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs.GetAt(i);
		for (INT_PTR j=i-1; j>=0; --j)
		{
			CRevisionEntry * preventry = (CRevisionEntry*)m_arEntryPtrs.GetAt(j);
			if ((reventry->level == preventry->level)&&(reventry->url == preventry->url))
			{
				// if an entry is added, then we don't connect anymore
				if ((preventry->action == CRevisionEntry::added)||(preventry->action == CRevisionEntry::addedwithhistory))
					break;
				// same level and url, now connect those two
				// but first check if they're not already connected!
				BOOL bConnected = FALSE;
				if ((reventry->action != CRevisionEntry::deleted)&&
					(preventry->action != CRevisionEntry::added)&&
					(preventry->action != CRevisionEntry::addedwithhistory)&&
					(preventry->action != CRevisionEntry::replaced))
				{
					for (INT_PTR k=0; k<reventry->sourcearray.GetCount(); ++k)
					{
						source_entry * s_entry = (source_entry *)reventry->sourcearray[k];
						if ((s_entry->revisionto == preventry->revision)&&(s_entry->pathto == preventry->url))
							bConnected = TRUE;
					}
					for (INT_PTR k=0; k<preventry->sourcearray.GetCount(); ++k)
					{
						source_entry * s_entry = (source_entry *)preventry->sourcearray[k];
						if ((s_entry->revisionto == reventry->revision)&&(s_entry->pathto ==  reventry->url))
							bConnected = TRUE;
					}
					if (!bConnected)
					{
						source_entry * sentry = new source_entry;
						sentry->pathto = preventry->url;
						sentry->revisionto = preventry->revision;
						reventry->sourcearray.Add(sentry);
						if (reventry->action == CRevisionEntry::lastcommit)
							reventry->action = CRevisionEntry::source;
						break;
					}
				}
			}
		}
	}

	// step five: adjust the entry levels
	qsort(m_arEntryPtrs.GetData(), m_arEntryPtrs.GetSize(), sizeof(CRevisionEntry *), (GENERICCOMPAREFN)SortCompareRevLevels);
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs[i];
		if (i<m_arEntryPtrs.GetCount()-1)
		{
			CRevisionEntry * reventry2 = (CRevisionEntry*)m_arEntryPtrs[i+1];
			if ((reventry2->revision == reventry->revision)&&(reventry2->level == reventry->level))
			{
				reventry2->level++;
				qsort(m_arEntryPtrs.GetData(), m_arEntryPtrs.GetSize(), sizeof(CRevisionEntry *), (GENERICCOMPAREFN)SortCompareRevLevels);
				i=-1;
			}
		}
	}

	// step six: sort the connections in each revision
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs[i];
		qsort(reventry->sourcearray.GetData(), reventry->sourcearray.GetSize(), sizeof(source_entry *), (GENERICCOMPAREFN)SortCompareSourceEntry);
	}
	
	return true;
}

int CRevisionGraph::SortCompareRevUrl(const void * pElem1, const void * pElem2)
{
	CRevisionEntry * entry1 = *((CRevisionEntry**)pElem1);
	CRevisionEntry * entry2 = *((CRevisionEntry**)pElem2);
	if (entry2->revision == entry1->revision)
	{
		return wcscmp(entry2->url, entry1->url);
	}
	return (entry2->revision - entry1->revision);
}

int CRevisionGraph::SortCompareRevLevels(const void * pElem1, const void * pElem2)
{
	CRevisionEntry * entry1 = *((CRevisionEntry**)pElem1);
	CRevisionEntry * entry2 = *((CRevisionEntry**)pElem2);
	if (entry2->revision == entry1->revision)
	{
		if (entry1->level == entry2->level)
			return wcscmp (entry1->url, entry2->url);
		return (entry1->level - entry2->level);
	}
	return (entry2->revision - entry1->revision);
}

int CRevisionGraph::SortCompareSourceEntry(const void * pElem1, const void * pElem2)
{
	source_entry * entry1 = *((source_entry**)pElem1);
	source_entry * entry2 = *((source_entry**)pElem2);
	if (entry1->revisionto == entry2->revisionto)
	{
		return wcscmp(entry2->pathto, entry1->pathto);
	}
	return (entry1->revisionto - entry2->revisionto);
}

bool CRevisionGraph::IsParentOrItself(const char * parent, const char * child)
{
	size_t len = strlen(parent);
	if (strncmp(parent, child, len)==0)
	{
		if ((child[len]=='/')||(child[len]==0))
			return true;
	}
	return false;
}

bool CRevisionGraph::IsParentOrItself(const wchar_t * parent, const wchar_t * child)
{
	size_t len = wcslen(parent);
	if (wcsncmp(parent, child, len)==0)
	{
		if ((child[len]=='/')||(child[len]==0))
			return true;
	}
	return false;
}

CString CRevisionGraph::GetLastErrorMessage()
{
	return SVN::GetErrorString(Err);
}

CString CRevisionGraph::GetRename(const CString& url, LONG rev)
{
	TLogDataMap::iterator iter = m_logdata.find (rev);
	if (iter != m_logdata.end())
	{
		log_entry * logentry = iter->second;

		for (INT_PTR i = 0, count = logentry->changes->GetCount(); i < count; ++i)
		{
			const LogChangedPath* change = logentry->changes->GetAt (i);

			if (   !change->sCopyFromPath.IsEmpty() 
				&& IsParentOrItself (change->sCopyFromPath, url))
			{
				// check if copyfrom_path was removed in this revision

				for (INT_PTR k = 0; k < count; ++k)
				{
					const LogChangedPath* change2 = logentry->changes->GetAt (k);

					if (   (change2->action == LOGACTIONS_DELETED)
						&& (change2->sPath == change->sCopyFromPath))
					{
						CString child = url.Mid (change2->sPath.GetLength());
						return change->sPath + child;
					}
				}	
			}
		}
	}
	return CString();
}

#ifdef DEBUG
void CRevisionGraph::PrintDebugInfo()
{
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		CRevisionEntry * entry = (CRevisionEntry *)m_arEntryPtrs[i];
		ATLTRACE("-------------------------------\n");
		ATLTRACE("entry        : %s\n", entry->path.GetPath());
		ATLTRACE("revision     : %ld\n", entry->revision);
		ATLTRACE("action       : %d\n", entry->action);
		ATLTRACE("level        : %d\n", entry->level);
		for (INT_PTR k=0; k<entry->sourcearray.GetCount(); ++k)
		{
			source_entry * sentry = (source_entry *)entry->sourcearray[k];
			ATLTRACE("pathto       : %s\n", sentry->pathto);
			ATLTRACE("revisionto   : %ld\n", sentry->revisionto);
		}
	}
		ATLTRACE("*******************************\n");
}
#endif
