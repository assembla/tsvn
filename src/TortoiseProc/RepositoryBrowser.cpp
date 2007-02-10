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
#include "stdafx.h"
#include "TortoiseProc.h"

#include "MessageBox.h"
#include "InputLogDlg.h"
#include "LogDlg.h"
#include "PropDlg.h"
#include "Blame.h"
#include "BlameDlg.h"
#include "WaitCursorEx.h"
#include "Repositorybrowser.h"
#include "BrowseFolder.h"
#include "RenameDlg.h"
#include "RevisionGraphDlg.h"
#include "CheckoutDlg.h"
#include "ExportDlg.h"
#include "SVNProgressDlg.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "TempFile.h"
#include "UnicodeUtils.h"
#include "BrowseFolder.h"
#include "SVNDiff.h"
#include "SysImageList.h"


enum RepoBrowserContextMenuCommands
{
	// needs to start with 1, since 0 is the return value if *nothing* is clicked on in the context menu
	ID_OPEN = 1,
	ID_OPENWITH,
	ID_SHOWLOG,
	ID_REVGRAPH,
	ID_BLAME,
	ID_VIEWREV,
	ID_VIEWPATHREV,
	ID_EXPORT,
	ID_CHECKOUT,
	ID_REFRESH,
	ID_SAVEAS,
	ID_MKDIR,
	ID_IMPORT,
	ID_IMPORTFOLDER,
	ID_BREAKLOCK,
	ID_DELETE,
	ID_RENAME,
	ID_COPYTOWC,
	ID_COPYTO,
	ID_URLTOCLIPBOARD,
	ID_PROPS,
	ID_GNUDIFF,
	ID_DIFF,

};



IMPLEMENT_DYNAMIC(CRepositoryBrowser, CResizableStandAloneDialog)

CRepositoryBrowser::CRepositoryBrowser(const SVNUrl& svn_url, BOOL bFile)
	: CResizableStandAloneDialog(CRepositoryBrowser::IDD, NULL)
	, m_cnrRepositoryBar(&m_barRepository)
	, m_bStandAlone(true)
	, m_InitialSvnUrl(svn_url)
	, m_bInitDone(false)
{
}

CRepositoryBrowser::CRepositoryBrowser(const SVNUrl& svn_url, CWnd* pParent, BOOL bFile)
	: CResizableStandAloneDialog(CRepositoryBrowser::IDD, pParent)
	, m_cnrRepositoryBar(&m_barRepository)
	, m_InitialSvnUrl(svn_url, true)
	, m_bStandAlone(false)
	, m_bInitDone(false)
{
}

CRepositoryBrowser::~CRepositoryBrowser()
{
}

void CRepositoryBrowser::RecursiveRemove(HTREEITEM hItem)
{
	HTREEITEM childItem;
	if (m_RepoTree.ItemHasChildren(hItem))
	{
		for (childItem = m_RepoTree.GetChildItem(hItem);childItem != NULL; childItem = m_RepoTree.GetNextItem(childItem, TVGN_NEXT))
		{
			RecursiveRemove(childItem);
			CTreeItem * pTreeItem = (CTreeItem*)m_RepoTree.GetItemData(childItem);
			delete pTreeItem;
			m_RepoTree.SetItemData(childItem, 0);
		}
	}

	CTreeItem * pTreeItem = (CTreeItem*)m_RepoTree.GetItemData(hItem);
	delete pTreeItem;
	m_RepoTree.SetItemData(hItem, 0);
}

void CRepositoryBrowser::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REPOTREE, m_RepoTree);
	DDX_Control(pDX, IDC_REPOLIST, m_RepoList);
}

BEGIN_MESSAGE_MAP(CRepositoryBrowser, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_WM_CONTEXTMENU()
	ON_WM_SETCURSOR()
	ON_REGISTERED_MESSAGE(WM_AFTERINIT, OnAfterInitDialog) 
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_NOTIFY(TVN_SELCHANGED, IDC_REPOTREE, &CRepositoryBrowser::OnTvnSelchangedRepotree)
	ON_NOTIFY(TVN_ITEMEXPANDING, IDC_REPOTREE, &CRepositoryBrowser::OnTvnItemexpandingRepotree)
END_MESSAGE_MAP()

SVNUrl CRepositoryBrowser::GetURL() const
{
	return m_barRepository.GetCurrentUrl();
}

SVNRev CRepositoryBrowser::GetRevision() const
{
	return GetURL().GetRevision();
}

CString CRepositoryBrowser::GetPath() const
{
	return GetURL().GetPath();
}

BOOL CRepositoryBrowser::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	bool bSortNumerical = !!(DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\SortNumerical"), TRUE);
	//m_treeRepository.SortNumerical(bSortNumerical);
	m_cnrRepositoryBar.SubclassDlgItem(IDC_REPOS_BAR_CNR, this);
	m_barRepository.Create(&m_cnrRepositoryBar, 12345);
	//m_barRepository.AssocTree(&m_treeRepository);

	if (m_bStandAlone)
	{
		GetDlgItem(IDCANCEL)->ShowWindow(FALSE);

		// reposition the buttons
		CRect rect_cancel;
		GetDlgItem(IDCANCEL)->GetWindowRect(rect_cancel);
		ScreenToClient(rect_cancel);
		GetDlgItem(IDOK)->MoveWindow(rect_cancel);
	}

	m_nIconFolder = SYS_IMAGE_LIST().GetDirIconIndex();
	m_nOpenIconFolder = SYS_IMAGE_LIST().GetDirOpenIconIndex();
	// set up the list control
	m_RepoList.SetExtendedStyle(LVS_EX_HEADERDRAGDROP | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP | LVS_EX_SUBITEMIMAGES);
	m_RepoList.SetImageList(&SYS_IMAGE_LIST(), LVSIL_SMALL);
	m_RepoList.ShowText(CString(MAKEINTRESOURCE(IDS_REPOBROWSE_INITWAIT)));

	m_RepoTree.SetImageList(&SYS_IMAGE_LIST(), TVSIL_NORMAL);

	AddAnchor(IDC_REPOS_BAR_CNR, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_F5HINT, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_REPOTREE, TOP_LEFT, BOTTOM_LEFT);
	AddAnchor(IDC_REPOLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	EnableSaveRestore(_T("RepositoryBrowser"));
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	m_bThreadRunning = true;
	if (AfxBeginThread(InitThreadEntry, this)==NULL)
	{
		m_bThreadRunning = false;
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	return TRUE;
}

UINT CRepositoryBrowser::InitThreadEntry(LPVOID pVoid)
{
	return ((CRepositoryBrowser*)pVoid)->InitThread();
}

//this is the thread function which calls the subversion function
UINT CRepositoryBrowser::InitThread()
{
	// In this thread, we try to find out the repository root.
	// Since this is a remote operation, it can take a while, that's
	// Why we do this inside a thread.

	// force the cursor to change
	POINT pt;
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);

	DialogEnableWindow(IDOK, FALSE);
	DialogEnableWindow(IDCANCEL, FALSE);
	m_strReposRoot = GetRepositoryRootAndUUID(CTSVNPath(m_InitialSvnUrl.GetPath()), m_sUUID);
	m_strReposRoot = SVNUrl::Unescape(m_strReposRoot);
	PostMessage(WM_AFTERINIT);
	DialogEnableWindow(IDOK, TRUE);
	DialogEnableWindow(IDCANCEL, TRUE);
	
	m_bThreadRunning = false;

	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);
	return 0;
}

LRESULT CRepositoryBrowser::OnAfterInitDialog(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	if (m_InitialSvnUrl.GetPath().IsEmpty())
		m_InitialSvnUrl = m_barRepository.GetCurrentUrl();

	ChangeToUrl(m_InitialSvnUrl.GetPath());
	m_barRepository.GotoUrl(m_InitialSvnUrl);
	m_RepoList.ClearText();
	m_bInitDone = TRUE;
	return 0;
}

void CRepositoryBrowser::OnOK()
{
	HTREEITEM hItem = m_RepoTree.GetRootItem();
	RecursiveRemove(hItem);

	m_barRepository.SaveHistory();
	CResizableStandAloneDialog::OnOK();
}

void CRepositoryBrowser::OnBnClickedHelp()
{
	OnHelp();
}

BOOL CRepositoryBrowser::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	if (pWnd == this)
	{
		RECT rect;
		POINT pt;
		GetClientRect(&rect);
		GetCursorPos(&pt);
		ScreenToClient(&pt);
		if (PtInRect(&rect, pt))
		{
			ClientToScreen(&pt);
			// are we right of the tree control?
			GetDlgItem(IDC_REPOTREE)->GetWindowRect(&rect);
			if ((pt.x > rect.right)&&
				(pt.y >= rect.top)&&
				(pt.y <= rect.bottom))
			{
				// but left of the list control?
				GetDlgItem(IDC_REPOLIST)->GetWindowRect(&rect);
				if (pt.x < rect.left)
				{
					HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_SIZEWE));
					SetCursor(hCur);
					return TRUE;
				}
			}
			CStandAloneDialogTmpl<CResizableDialog>::OnSetCursor(pWnd, nHitTest, message);
		}
	}
	return FALSE;
}

void CRepositoryBrowser::OnMouseMove(UINT nFlags, CPoint point)
{
	CDC * pDC;
	RECT rect, tree, list, treelist, treelistclient;

	if (bDragMode == FALSE)
		return;

	// create an union of the tree and list control rectangle
	GetDlgItem(IDC_REPOLIST)->GetWindowRect(&list);
	GetDlgItem(IDC_REPOTREE)->GetWindowRect(&tree);
	UnionRect(&treelist, &tree, &list);
	treelistclient = treelist;
	ScreenToClient(&treelistclient);

	//convert the mouse coordinates relative to the top-left of
	//the window
	ClientToScreen(&point);
	GetClientRect(&rect);
	ClientToScreen(&rect);
	point.x -= rect.left;
	point.y -= treelist.top;

	//same for the window coordinates - make them relative to 0,0
	OffsetRect(&treelist, -treelist.left, -treelist.top);

	if (point.x < treelist.left+REPOBROWSER_CTRL_MIN_WIDTH)
		point.x = treelist.left+REPOBROWSER_CTRL_MIN_WIDTH;
	if (point.x > treelist.right-REPOBROWSER_CTRL_MIN_WIDTH) 
		point.x = treelist.right-REPOBROWSER_CTRL_MIN_WIDTH;

	if ((nFlags & MK_LBUTTON) && (point.x != oldx))
	{
		pDC = GetDC();

		if (pDC)
		{
			DrawXorBar(pDC, oldx+2, treelistclient.top, 4, treelistclient.bottom-treelistclient.top-2);
			DrawXorBar(pDC, point.x+2, treelistclient.top, 4, treelistclient.bottom-treelistclient.top-2);

			ReleaseDC(pDC);
		}

		oldx = point.x;
		oldy = point.y;
	}

	CStandAloneDialogTmpl<CResizableDialog>::OnMouseMove(nFlags, point);
}

void CRepositoryBrowser::OnLButtonDown(UINT nFlags, CPoint point)
{
	CDC * pDC;
	RECT rect, tree, list, treelist, treelistclient;

	// create an union of the tree and list control rectangle
	GetDlgItem(IDC_REPOLIST)->GetWindowRect(&list);
	GetDlgItem(IDC_REPOTREE)->GetWindowRect(&tree);
	UnionRect(&treelist, &tree, &list);
	treelistclient = treelist;
	ScreenToClient(&treelistclient);

	//convert the mouse coordinates relative to the top-left of
	//the window
	ClientToScreen(&point);
	GetClientRect(&rect);
	ClientToScreen(&rect);
	point.x -= rect.left;
	point.y -= treelist.top;

	//same for the window coordinates - make them relative to 0,0
	OffsetRect(&treelist, -treelist.left, -treelist.top);

	if (point.x < treelist.left+REPOBROWSER_CTRL_MIN_WIDTH)
		point.x = treelist.left+REPOBROWSER_CTRL_MIN_WIDTH;
	if (point.x > treelist.right-REPOBROWSER_CTRL_MIN_WIDTH) 
		point.x = treelist.right-REPOBROWSER_CTRL_MIN_WIDTH;

	if ((point.y < treelist.top) || 
		(point.y > treelist.bottom))
		return CStandAloneDialogTmpl<CResizableDialog>::OnLButtonDown(nFlags, point);

	bDragMode = true;

	SetCapture();

	pDC = GetDC();
	DrawXorBar(pDC, point.x+2, treelistclient.top, 4, treelistclient.bottom-treelistclient.top-2);
	ReleaseDC(pDC);

	oldx = point.x;
	oldy = point.y;

	CStandAloneDialogTmpl<CResizableDialog>::OnLButtonDown(nFlags, point);
}

void CRepositoryBrowser::OnLButtonUp(UINT nFlags, CPoint point)
{
	CDC * pDC;
	RECT rect, tree, list, treelist, treelistclient;

	if (bDragMode == FALSE)
		return;

	// create an union of the tree and list control rectangle
	GetDlgItem(IDC_REPOLIST)->GetWindowRect(&list);
	GetDlgItem(IDC_REPOTREE)->GetWindowRect(&tree);
	UnionRect(&treelist, &tree, &list);
	treelistclient = treelist;
	ScreenToClient(&treelistclient);

	ClientToScreen(&point);
	GetClientRect(&rect);
	ClientToScreen(&rect);

	CPoint point2 = point;
	if (point2.x < treelist.left+REPOBROWSER_CTRL_MIN_WIDTH)
		point2.x = treelist.left+REPOBROWSER_CTRL_MIN_WIDTH;
	if (point2.x > treelist.right-REPOBROWSER_CTRL_MIN_WIDTH) 
		point2.x = treelist.right-REPOBROWSER_CTRL_MIN_WIDTH;

	point.x -= rect.left;
	point.y -= treelist.top;

	OffsetRect(&treelist, -treelist.left, -treelist.top);

	if (point.x < treelist.left+REPOBROWSER_CTRL_MIN_WIDTH)
		point.x = treelist.left+REPOBROWSER_CTRL_MIN_WIDTH;
	if (point.x > treelist.right-REPOBROWSER_CTRL_MIN_WIDTH) 
		point.x = treelist.right-REPOBROWSER_CTRL_MIN_WIDTH;

	pDC = GetDC();
	DrawXorBar(pDC, oldx+2, treelistclient.top, 4, treelistclient.bottom-treelistclient.top-2);			
	ReleaseDC(pDC);

	oldx = point.x;
	oldy = point.y;

	bDragMode = false;
	ReleaseCapture();

	//position the child controls
	GetDlgItem(IDC_REPOTREE)->GetWindowRect(&treelist);
	treelist.right = point2.x - 2;
	ScreenToClient(&treelist);
	RemoveAnchor(IDC_REPOTREE);
	GetDlgItem(IDC_REPOTREE)->MoveWindow(&treelist);
	GetDlgItem(IDC_REPOLIST)->GetWindowRect(&treelist);
	treelist.left = point2.x + 2;
	ScreenToClient(&treelist);
	RemoveAnchor(IDC_REPOLIST);
	GetDlgItem(IDC_REPOLIST)->MoveWindow(&treelist);

	AddAnchor(IDC_REPOTREE, TOP_LEFT, BOTTOM_LEFT);
	AddAnchor(IDC_REPOLIST, TOP_LEFT, BOTTOM_RIGHT);

	CStandAloneDialogTmpl<CResizableDialog>::OnLButtonUp(nFlags, point);
}

void CRepositoryBrowser::DrawXorBar(CDC * pDC, int x1, int y1, int width, int height)
{
	static WORD _dotPatternBmp[8] = 
	{ 
		0x0055, 0x00aa, 0x0055, 0x00aa, 
		0x0055, 0x00aa, 0x0055, 0x00aa
	};

	HBITMAP hbm;
	HBRUSH  hbr, hbrushOld;

	hbm = CreateBitmap(8, 8, 1, 1, _dotPatternBmp);
	hbr = CreatePatternBrush(hbm);

	pDC->SetBrushOrg(x1, y1);
	hbrushOld = (HBRUSH)pDC->SelectObject(hbr);

	PatBlt(pDC->GetSafeHdc(), x1, y1, width, height, PATINVERT);

	pDC->SelectObject(hbrushOld);

	DeleteObject(hbr);
	DeleteObject(hbm);
}

BOOL CRepositoryBrowser::ReportList(const CString& path, svn_node_kind_t kind, 
									svn_filesize_t size, bool has_props, 
									svn_revnum_t created_rev, apr_time_t time, 
									const CString& author, const CString& locktoken, 
									const CString& lockowner, const CString& lockcomment, 
									bool is_dav_comment, apr_time_t lock_creationdate, 
									apr_time_t lock_expirationdate, 
									const CString& absolutepath)
{
	static deque<CItem> * pDirList = NULL;
	static CTreeItem * pTreeItem = NULL;
	static CString dirPath;

	CString sParent = absolutepath;
	int slashpos = path.ReverseFind('/');
	bool abspath_has_slash = (absolutepath.GetAt(absolutepath.GetLength()-1) == '/');
	if ((slashpos > 0) && (!abspath_has_slash))
		sParent += _T("/");
	sParent += path.Left(slashpos);
	if ((path.IsEmpty())||
		(pDirList == NULL)||
		(sParent.Compare(dirPath)))
	{
		if (sParent.Compare(_T("/"))==0)
			sParent.Empty();
		HTREEITEM hItem = FindUrl(m_strReposRoot + sParent);
		pTreeItem = (CTreeItem*)m_RepoTree.GetItemData(hItem);
		pDirList = &(pTreeItem->children);

		dirPath = sParent;
	}
	if (path.IsEmpty())
		return TRUE;

	if (kind == svn_node_dir)
	{
		FindUrl(m_strReposRoot + absolutepath + (abspath_has_slash ? _T("") : _T("/")) + path);
		if (pTreeItem)
			pTreeItem->has_child_folders = true;
	}
	pDirList->push_back(CItem(path.Mid(slashpos+1), kind, size, has_props,
		created_rev, time, author, locktoken,
		lockowner, lockcomment, is_dav_comment,
		lock_creationdate, lock_expirationdate,
		absolutepath+path.Left(slashpos)));

	return TRUE;
}

bool CRepositoryBrowser::ChangeToUrl(const CString& url)
{
	CString partUrl = url;
	HTREEITEM hItem = m_RepoTree.GetRootItem();
	if (hItem == NULL)
	{
		// the tree view is empty, just fill in the repository root
		CTreeItem * pTreeItem = new CTreeItem();
		pTreeItem->unescapedname = m_strReposRoot;
		pTreeItem->url = m_strReposRoot;

		TVINSERTSTRUCT tvinsert = {0};
		tvinsert.hParent = TVI_ROOT;
		tvinsert.hInsertAfter = TVI_ROOT;
		tvinsert.itemex.mask = TVIF_CHILDREN | TVIF_DI_SETITEM | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvinsert.itemex.state = TVIS_EXPANDED;
		tvinsert.itemex.stateMask = TVIS_EXPANDED;
		tvinsert.itemex.pszText = m_strReposRoot.GetBuffer(m_strReposRoot.GetLength());
		tvinsert.itemex.cChildren = 1;
		tvinsert.itemex.lParam = (LPARAM)pTreeItem;
		tvinsert.itemex.iImage = m_nIconFolder;
		tvinsert.itemex.iSelectedImage = m_nOpenIconFolder;

		hItem = m_RepoTree.InsertItem(&tvinsert);
		m_strReposRoot.ReleaseBuffer();
	}
	if (hItem == NULL)
	{
		// something terrible happened!
		return false;
	}
	hItem = FindUrl(url);
	if (hItem == NULL)
		return false;

	CTreeItem * pTreeItem = (CTreeItem *)m_RepoTree.GetItemData(hItem);
	if (pTreeItem == NULL)
		return FALSE;

	m_RepoList.ShowText(_T(" "), true);

	RefreshNode(hItem);

	FillList(&pTreeItem->children);
	m_RepoList.ClearText();

	return true;
}

void CRepositoryBrowser::FillList(deque<CItem> * pItems)
{
	if (pItems == NULL)
		return;
	m_RepoList.SetRedraw(false);
	m_RepoList.DeleteAllItems();

	int c = ((CHeaderCtrl*)(m_RepoList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_RepoList.DeleteColumn(c--);

	c = 0;
	CString temp;
	//
	// column 0: contains tree
	temp.LoadString(IDS_LOG_FILE);
	m_RepoList.InsertColumn(c++, temp);
	//
	// column 1: file extension
	temp.LoadString(IDS_STATUSLIST_COLEXT);
	m_RepoList.InsertColumn(c++, temp);
	//
	// column 2: revision number
	temp.LoadString(IDS_LOG_REVISION);
	m_RepoList.InsertColumn(c++, temp);
	//
	// column 3: author
	temp.LoadString(IDS_LOG_AUTHOR);
	m_RepoList.InsertColumn(c++, temp);
	//
	// column 4: size
	temp.LoadString(IDS_LOG_SIZE);
	m_RepoList.InsertColumn(c++, temp);
	//
	// column 5: date
	temp.LoadString(IDS_LOG_DATE);
	m_RepoList.InsertColumn(c++, temp);
	//
	// column 6: lock owner
	temp.LoadString(IDS_STATUSLIST_COLLOCK);
	m_RepoList.InsertColumn(c++, temp);

	// now fill in the data

	int nCount = 0;
	for (deque<CItem>::const_iterator it = pItems->begin(); it != pItems->end(); ++it)
	{
		int icon_idx;
		if (it->kind == svn_node_dir)
			icon_idx = 	m_nIconFolder;
		else
			icon_idx = SYS_IMAGE_LIST().GetFileIconIndex(it->path);
		int index = m_RepoList.InsertItem(nCount, it->path, icon_idx);
		// extension
		temp.Format(_T("%ld"), it->created_rev);
		m_RepoList.SetItemText(index, 2, temp);
		m_RepoList.SetItemText(index, 3, it->author);
		temp.Format(_T("%ld"), it->size);
		m_RepoList.SetItemText(index, 4, temp);
		// date
		m_RepoList.SetItemText(index, 6, it->lockowner);
	}

	for (int col = 0; col <= (((CHeaderCtrl*)(m_RepoList.GetDlgItem(0)))->GetItemCount()-1); col++)
	{
		m_RepoList.SetColumnWidth(col, LVSCW_AUTOSIZE_USEHEADER);
	}

	m_RepoList.SetRedraw(true);


}

HTREEITEM CRepositoryBrowser::FindUrl(const CString& fullurl, bool create /* = true */)
{
	return FindUrl(fullurl, fullurl, create, TVI_ROOT);
}

HTREEITEM CRepositoryBrowser::FindUrl(const CString& fullurl, const CString& url, bool create /* true */, HTREEITEM hItem /* = TVI_ROOT */)
{
	if (hItem == TVI_ROOT)
	{
		hItem = m_RepoTree.GetRootItem();
		if (fullurl.Compare(m_strReposRoot)==0)
			return hItem;
		return FindUrl(fullurl, url.Mid(m_strReposRoot.GetLength()+1), create, hItem);
	}
	HTREEITEM hSibling = hItem;
	if (m_RepoTree.GetNextItem(hItem, TVGN_CHILD))
		hSibling = m_RepoTree.GetNextItem(hItem, TVGN_CHILD);
	do
	{
		CString sSibling = ((CTreeItem*)m_RepoTree.GetItemData(hSibling))->unescapedname;
		if (sSibling.Compare(url.Left(sSibling.GetLength()))==0)
		{
			if (sSibling.GetLength() == url.GetLength())
				return hSibling;
			if (url.GetAt(sSibling.GetLength()) == '/')
				return FindUrl(fullurl, url.Mid(sSibling.GetLength()+1), create, hSibling);
		}
	} while ((hSibling = m_RepoTree.GetNextItem(hSibling, TVGN_NEXT)) != NULL);	
	if (!create)
		return NULL;
	// create tree items for every path part in the url
	CString sUrl = url;
	int slash = -1;
	HTREEITEM hNewItem = hItem;
	CString sTemp;
	while ((slash=sUrl.Find('/')) >= 0)
	{
		CTreeItem * pTreeItem = new CTreeItem();
		sTemp = sUrl.Left(slash);
		pTreeItem->unescapedname = sTemp;
		pTreeItem->url = fullurl.Left(fullurl.GetLength()-sUrl.GetLength()+slash);
		TVINSERTSTRUCT tvinsert = {0};
		tvinsert.hParent = hNewItem;
		tvinsert.hInsertAfter = TVI_LAST;
		tvinsert.itemex.mask = TVIF_CHILDREN | TVIF_DI_SETITEM | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvinsert.itemex.state = TVIS_EXPANDED;
		tvinsert.itemex.stateMask = TVIS_EXPANDED;
		tvinsert.itemex.pszText = sTemp.GetBuffer(sTemp.GetLength());
		tvinsert.itemex.cChildren = 1;
		tvinsert.itemex.lParam = (LPARAM)pTreeItem;
		tvinsert.itemex.iImage = m_nIconFolder;
		tvinsert.itemex.iSelectedImage = m_nOpenIconFolder;

		hNewItem = m_RepoTree.InsertItem(&tvinsert);
		sTemp.ReleaseBuffer();
		sUrl = sUrl.Mid(slash+1);
		ATLTRACE("created tree entry %ws, url %ws\n", sTemp, pTreeItem->url);
	}
	CTreeItem * pTreeItem = new CTreeItem();
	sTemp = sUrl;
	pTreeItem->unescapedname = sTemp;
	pTreeItem->url = fullurl;
	TVINSERTSTRUCT tvinsert = {0};
	tvinsert.hParent = hNewItem;
	tvinsert.hInsertAfter = TVI_LAST;
	tvinsert.itemex.mask = TVIF_CHILDREN | TVIF_DI_SETITEM | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvinsert.itemex.state = TVIS_EXPANDED;
	tvinsert.itemex.stateMask = TVIS_EXPANDED;
	tvinsert.itemex.pszText = sTemp.GetBuffer(sTemp.GetLength());
	tvinsert.itemex.cChildren = 1;
	tvinsert.itemex.lParam = (LPARAM)pTreeItem;
	tvinsert.itemex.iImage = m_nIconFolder;
	tvinsert.itemex.iSelectedImage = m_nOpenIconFolder;

	hNewItem = m_RepoTree.InsertItem(&tvinsert);
	sTemp.ReleaseBuffer();
	ATLTRACE("created tree entry %ws, url %ws\n", sTemp, pTreeItem->url);
	return hNewItem;
}

void CRepositoryBrowser::OnCancel()
{
	HTREEITEM hItem = m_RepoTree.GetRootItem();
	RecursiveRemove(hItem);

	__super::OnCancel();
}

bool CRepositoryBrowser::RefreshNode(const CString& url)
{
	HTREEITEM hNode = FindUrl(url);
	return RefreshNode(hNode);
}

bool CRepositoryBrowser::RefreshNode(HTREEITEM hNode)
{
	CWaitCursorEx wait;
	CTreeItem * pTreeItem = (CTreeItem *)m_RepoTree.GetItemData(hNode);
	if (m_RepoTree.ItemHasChildren(hNode))
	{
		HTREEITEM hChild = m_RepoTree.GetChildItem(hNode);
		HTREEITEM hNext;
		while (hChild)
		{
			hNext = m_RepoTree.GetNextItem(hChild, TVGN_NEXT);
			RecursiveRemove(hChild);
			m_RepoTree.DeleteItem(hChild);
			hChild = hNext;
		}
	}
	pTreeItem->children.clear();
	pTreeItem->has_child_folders = false;
	if (!List(CTSVNPath(pTreeItem->url), GetRevision(), GetRevision(), true, true))
	{
		// error during list()
		m_RepoList.ShowText(GetLastErrorMessage());
		return false;
	}
	pTreeItem->children_fetched = true;
	// if there are no child folders, remove the '+' in front of the node
	if (!pTreeItem->has_child_folders)
	{
		TVITEM tvitem = {0};
		tvitem.hItem = hNode;
		tvitem.mask = TVIF_CHILDREN;
		tvitem.cChildren = 0;
		m_RepoTree.SetItem(&tvitem);
	}
	return true;
}

void CRepositoryBrowser::OnTvnSelchangedRepotree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	CTreeItem * pTreeItem = (CTreeItem *)pNMTreeView->itemNew.lParam;
	if (pTreeItem)
	{
		if (!pTreeItem->children_fetched)
		{
			m_RepoList.ShowText(_T(" "), true);
			RefreshNode(pNMTreeView->itemNew.hItem);
			m_RepoList.ClearText();
		}

		FillList(&pTreeItem->children);
	}

	*pResult = 0;
}

void CRepositoryBrowser::OnTvnItemexpandingRepotree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	// user wants to expand a tree node.
	// check if we already know its children - if not we have to ask the repository!

	CTreeItem * pTreeItem = (CTreeItem *)pNMTreeView->itemNew.lParam;
	if (!pTreeItem->children_fetched)
	{
		RefreshNode(pNMTreeView->itemNew.hItem);
	}

	*pResult = 0;
}
