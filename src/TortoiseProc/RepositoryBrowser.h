// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
#pragma once

#include <map>
#include <deque>

#include "SVNUrl.h"
#include "RepositoryTree.h"
#include "RepositoryBar.h"
#include "StandAloneDlg.h"
#include "ProjectProperties.h"
#include "LogDlg.h"
#include "TSVNPath.h"

#define REPOBROWSER_CTRL_MIN_WIDTH 20

using namespace std;

class CInputLogDlg;

class CItem
{
public:
	CItem() : kind(svn_node_none)
		, size(0)
		, has_props(false)
		, created_rev(0)
		, time(0)
		, is_dav_comment(false)
		, lock_creationdate(0)
		, lock_expirationdate(0)
	{

	}
	CItem(const CString& _path, 
		svn_node_kind_t _kind,
		svn_filesize_t _size,
		bool _has_props,
		svn_revnum_t _created_rev,
		apr_time_t _time,
		const CString& _author,
		const CString& _locktoken,
		const CString& _lockowner,
		const CString& _lockcomment,
		bool _is_dav_comment,
		apr_time_t _lock_creationdate,
		apr_time_t _lock_expirationdate,
		const CString& _absolutepath)
	{
		path = _path;
		kind = _kind;
		size = _size;
		has_props = _has_props;
		created_rev = _created_rev;
		time = _time;
		author = _author;
		locktoken = _locktoken;
		lockowner = _lockowner;
		lockcomment = _lockcomment;
		is_dav_comment = _is_dav_comment;
		lock_creationdate = _lock_creationdate;
		lock_expirationdate = _lock_expirationdate;
		absolutepath = _absolutepath;
	}
public:
	CString				path;
	svn_node_kind_t		kind;
	svn_filesize_t		size;
	bool				has_props;
	svn_revnum_t		created_rev;
	apr_time_t			time;
	CString				author;
	CString				locktoken;
	CString				lockowner;
	CString				lockcomment;
	bool				is_dav_comment;
	apr_time_t			lock_creationdate;
	apr_time_t			lock_expirationdate;
	CString				absolutepath;
};

class CTreeItem
{
public:
	CTreeItem() : children_fetched(false) {}

	CString			unescapedname;
	CString			url;
	bool			children_fetched;			///< whether the contents of the folder are known/fetched or not
	deque<CItem>	children;
};

/**
 * \ingroup TortoiseProc
 * Dialog to browse a repository.
 */
class CRepositoryBrowser : public CResizableStandAloneDialog, public SVN
{
	DECLARE_DYNAMIC(CRepositoryBrowser)

public:
	CRepositoryBrowser(const SVNUrl& svn_url, BOOL bFile = FALSE);					///< standalone repository browser
	CRepositoryBrowser(const SVNUrl& svn_url, CWnd* pParent, BOOL bFile = FALSE);	///< dependent repository browser
	virtual ~CRepositoryBrowser();

	/// Returns the currently displayed URL and revision.
	SVNUrl GetURL() const;
	/// Returns the currently displayed revision only (for convenience)
	SVNRev GetRevision() const;
	/// Returns the currently displayed URL's path only (for convenience)
	CString GetPath() const;

	enum { IDD = IDD_REPOSITORY_BROWSER };

	ProjectProperties m_ProjectProperties;
	CTSVNPath m_path;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	afx_msg void OnBnClickedHelp();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);

	LRESULT OnAfterInitDialog(WPARAM /*wParam*/, LPARAM /*lParam*/);
	void DrawXorBar(CDC * pDC, int x1, int y1, int width, int height);
	virtual BOOL ReportList(const CString& path, svn_node_kind_t kind, 
		svn_filesize_t size, bool has_props, svn_revnum_t created_rev, 
		apr_time_t time, const CString& author, const CString& locktoken, 
		const CString& lockowner, const CString& lockcomment, 
		bool is_dav_comment, apr_time_t lock_creationdate, 
		apr_time_t lock_expirationdate, const CString& absolutepath);

	void RecursiveRemove(HTREEITEM hItem);
	bool ChangeToUrl(const CString& url);
	HTREEITEM FindUrl(const CString& fullurl, bool create = true);
	HTREEITEM FindUrl(const CString& fullurl, const CString& url, bool create = true, HTREEITEM hItem = TVI_ROOT);
	void FillList(deque<CItem> * pItems);

	DECLARE_MESSAGE_MAP()


	static UINT InitThreadEntry(LPVOID pVoid);
	UINT InitThread();

	bool				m_bInitDone;
	CRepositoryBar		m_barRepository;
	CRepositoryBarCnr	m_cnrRepositoryBar;

	CTreeCtrl			m_RepoTree;
	CListCtrl			m_RepoList;

	CString				m_strReposRoot;
	CString				m_sUUID;

private:
	bool m_bStandAlone;
	SVNUrl m_InitialSvnUrl;
	bool m_bThreadRunning;
	static const UINT	m_AfterInitMessage;

	int					m_nIconFolder;
	int					m_nOpenIconFolder;

	int					oldy, oldx;
	bool				bDragMode;
public:
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
protected:
	virtual void OnCancel();
public:
	afx_msg void OnTvnSelchangedRepotree(NMHDR *pNMHDR, LRESULT *pResult);
};

/**
 * \ingroup TortoiseProc
 * helper class to find the copyfrom revision of a tag/branch
 * and the corresponding copyfrom URL.
 */
class LogHelper : public SVN
{
public:
	virtual BOOL Log(LONG rev, const CString& /*author*/, const CString& /*date*/, const CString& /*message*/, LogChangedPathArray * cpaths, apr_time_t /*time*/, int /*filechanges*/, BOOL /*copies*/, DWORD /*actions*/)
	{
		m_rev = rev;
		for (int i=0; i<cpaths->GetCount(); ++i)
		{
			LogChangedPath * cpath = cpaths->GetAt(i);
			if (m_relativeurl.Compare(cpath->sPath)== 0)
			{
				m_copyfromurl = m_reposroot + cpath->sCopyFromPath;
				m_rev = cpath->lCopyFromRev;
			}
			delete cpath;
		}
		delete cpaths;
		return TRUE;
	}
	
	SVNRev GetCopyFromRev(CTSVNPath url, CString& copyfromURL, SVNRev pegrev)
	{
		SVNRev rev;
		m_reposroot = GetRepositoryRoot(url);
		m_relativeurl = url.GetSVNPathString().Mid(m_reposroot.GetLength());
		if (ReceiveLog(CTSVNPathList(url), pegrev, SVNRev::REV_HEAD, 1, 0, TRUE, TRUE))
		{
			rev = m_rev;
			copyfromURL = m_copyfromurl;
		}
		return rev;
	}
	CString m_reposroot;
	CString m_relativeurl;
	SVNRev m_rev;
	CString m_copyfromurl;
};
static UINT WM_AFTERINIT = RegisterWindowMessage(_T("TORTOISESVN_AFTERINIT_MSG"));

