// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN

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
#pragma once

#include "SVNPrompt.h"
#include "SVNLogQuery.h"
#include "CacheLogQuery.h"

#include <map>

typedef int (__cdecl *GENERICCOMPAREFN)(const void * elem1, const void * elem2);

class log_entry
{
public:

	std::auto_ptr<LogChangedPathArray> changes;
	svn_revnum_t rev;
	CString author;
	apr_time_t timeStamp;
	CString message;

	// default construction

	log_entry() {};

private:

	// copy is not allowed (due to std::auto_ptr)

	log_entry (const log_entry&);
	log_entry& operator= (const log_entry&);
};

class CRevisionEntry;

struct SCopyInfo
{
	revision_t fromRevision;
	index_t fromPathIndex;
	revision_t toRevision;
	index_t toPathIndex;

	struct STarget
	{
		CRevisionEntry* source;
		CDictionaryBasedTempPath path;

		STarget ( CRevisionEntry* source
				, const CDictionaryBasedTempPath& path)
			: source (source)
			, path (path)
		{
		}
	};

	std::vector<STarget> targets;
};

class CSearchPathTree
{
private:

	CDictionaryBasedTempPath path;

	revision_t startRevision;
	CRevisionEntry* lastEntry;

	CSearchPathTree* parent;
	CSearchPathTree* firstChild;
	CSearchPathTree* lastChild;
	CSearchPathTree* previous;
	CSearchPathTree* next;

	// utilities: handle node insertion / removal

	void DeLink();
	void Link (CSearchPathTree* newParent);

public:

	// construction / destruction

	CSearchPathTree (const CPathDictionary* dictionary);
	CSearchPathTree ( const CDictionaryBasedTempPath& path
					, revision_t startrev
					, CSearchPathTree* parent);

	~CSearchPathTree();

	// add a node for the given path and rev. to the tree

	CSearchPathTree* Insert ( const CDictionaryBasedTempPath& path
							, revision_t startrev);
	void Remove();

	// there is a new revision entry for this path

	void ChainEntries (CRevisionEntry* entry);

	// property access

	const CDictionaryBasedTempPath& GetPath() const
	{
		return path;
	}

	revision_t GetStartRevision() const
	{
		return startRevision;
	}

	CRevisionEntry* GetLastEntry() const
	{
		return lastEntry;
	}

	CSearchPathTree* GetParent() const
	{
		return parent;
	}

	CSearchPathTree* GetFirstChild() const
	{
		return firstChild;
	}

	CSearchPathTree* GetLastChild() const
	{
		return lastChild;
	}

	CSearchPathTree* GetNext() const
	{
		return next;
	}

	CSearchPathTree* GetPrevious() const
	{
		return previous;
	}

	bool IsActive() const
	{
		return startRevision != NO_REVISION;
	}

	bool IsEmpty() const
	{
		return !IsActive() && (firstChild == NULL);
	}

};

/**
 * \ingroup TortoiseProc
 * Helper class, representing a revision with all the required information
 * which we need to draw a revision graph.
 */
class CRevisionEntry
{
public:
	enum Action
	{
		nothing = 0,
		modified = 8,
		deleted = 32,
		added = 4,
		addedwithhistory = 5,
		renamed = 49,
		replaced = 17,
		lastcommit = 50,
		initial = 51,
		source = 1
	};
	//methods
	CRevisionEntry ( const CDictionaryBasedTempPath& path
				   , revision_t revision
				   , Action action)
		: path (path), realPath (path.GetBasePath()), revision (revision), action (action)
		, next (NULL)
		, row (0), column (0) {};

	//members
	revision_t		revision;
	CDictionaryBasedTempPath path;
	CDictionaryBasedPath realPath;

	Action			action;
	CRevisionEntry* next;

	std::vector<CRevisionEntry*>	copyTargets;

	size_t			index;

	int				column;
	int				row;

	CRect			drawrect;
};

/**
 * \ingroup TortoiseProc
 * Handles and analyzes log data to produce a revision graph.
 * 
 * Since Subversion only stores information where each entry is copied \b from
 * and not where it is copied \b to, the first thing we do here is crawl all
 * revisions and create separate CRevisionEntry objects where we store the
 * information where those are copied \b to.
 *
 * In a next step, we go again through all the CRevisionEntry objects to find
 * out if they are related to the path we're looking at. If they are, we mark
 * them as \b in-use.
 */
class CRevisionGraph : private ILogReceiver
{
public:
	CRevisionGraph(void);
	~CRevisionGraph(void);
	BOOL						FetchRevisionData(CString path);
	BOOL						AnalyzeRevisionData(CString path, bool bShowAll = false, bool bArrangeByPath = false);
	virtual BOOL				ProgressCallback(CString text1, CString text2, DWORD done, DWORD total);
	svn_revnum_t				GetHeadRevision() {return m_lHeadRevision;}
	bool						SetFilter(svn_revnum_t minrev, svn_revnum_t maxrev, const CString& sPathFilter);

	CString						GetLastErrorMessage();
	static bool					IsParentOrItself(const char * parent, const char * child);
	static bool					IsParentOrItself(const wchar_t * parent, const wchar_t * child);
	std::vector<CRevisionEntry*> m_entryPtrs;
	size_t						m_maxurllength;
	CString						m_maxurl;
	int							m_maxColumn;
	int							m_maxRow;

	std::auto_ptr<CSVNLogQuery> svnQuery;
	std::auto_ptr<CCacheLogQuery> query;

	CString						GetReposRoot() {return CString(m_sRepoRoot);}

	BOOL						m_bCancelled;

	apr_pool_t *				pool;			///< memory pool
	svn_client_ctx_t 			m_ctx;
	SVNPrompt					m_prompt;

private:

	typedef std::vector<SCopyInfo*>::const_iterator TSCopyIterator;

	void						BuildForwardCopies();
	void						AnalyzeRevisions ( const CDictionaryBasedTempPath& url
												 , revision_t startrev
												 , bool bShowAll);
	void						AnalyzeRevisions ( revision_t revision
												 , CRevisionInfoContainer::CChangesIterator first
												 , CRevisionInfoContainer::CChangesIterator last
												 , CSearchPathTree* startNode
												 , bool bShowAll
												 , std::vector<CSearchPathTree*>& toRemove);
	void						AddCopiedPaths ( revision_t revision
											   , CSearchPathTree* rootNode
											   , TSCopyIterator& lastToCopy);
	void						FillCopyTargets ( revision_t revision
											    , CSearchPathTree* rootNode
											    , TSCopyIterator& lastFromCopy);
	void						AssignColumns ( CRevisionEntry* start
											  , std::vector<int>& columnByRow
                                              , int column);
	void						Optimize();
	int							AssignOneRowPerRevision();
	int							AssignOneRowPerBranchNode (CRevisionEntry* start, int row);
	void						AssignCoordinates (bool groupBranches);
	void						Cleanup();
	void						ClearRevisionEntries();
	
	CStringA					m_sRepoRoot;
	revision_t					m_lHeadRevision;

	std::set<std::wstring>		m_filterpaths;
	svn_revnum_t				m_FilterMinRev;
	svn_revnum_t				m_FilterMaxRev;

	int							m_nRecurseLevel;
	svn_error_t *				Err;			///< Global error object struct
	apr_pool_t *				parentpool;
	static svn_error_t*			cancel(void *baton);

	std::vector<SCopyInfo*>		copyToRelation;
	std::vector<SCopyInfo*>		copyFromRelation;

	// implement ILogReceiver

	void ReceiveLog ( LogChangedPathArray* changes
					, svn_revnum_t rev
					, const CString& author
					, const apr_time_t& timeStamp
					, const CString& message);

};
