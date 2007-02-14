
#include "StdAfx.h"
#include "RepoDrags.h"
#include "RepositoryBrowser.h"

CTreeDropTarget::CTreeDropTarget(CRepositoryBrowser * pRepoBrowser) : CIDropTarget(pRepoBrowser->m_RepoTree.GetSafeHwnd())
	, m_pRepoBrowser(pRepoBrowser)
{
}

bool CTreeDropTarget::OnDrop(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD *pdwEffect)
{
	if (pFmtEtc->cfFormat == CF_UNICODETEXT && medium.tymed == TYMED_HGLOBAL)
	{
		TCHAR* pStr = (TCHAR*)GlobalLock(medium.hGlobal);
		CString urls;
		if(pStr != NULL)
		{
			urls = pStr;
		}
		GlobalUnlock(medium.hGlobal);
		urls.Replace(_T("\r\n"), _T("*"));
		CTSVNPathList urlList;
		urlList.LoadFromAsteriskSeparatedString(urls);
		m_pRepoBrowser->OnDrop(urlList);
	}

	if(pFmtEtc->cfFormat == CF_HDROP && medium.tymed == TYMED_HGLOBAL)
	{
		HDROP hDrop = (HDROP)GlobalLock(medium.hGlobal);
		if(hDrop != NULL)
		{
			CTSVNPathList urlList;
			TCHAR szFileName[MAX_PATH];

			UINT cFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0); 
			for(UINT i = 0; i < cFiles; ++i)
			{
				DragQueryFile(hDrop, i, szFileName, sizeof(szFileName));
				urlList.AddPath(CTSVNPath(szFileName));
			}
			m_pRepoBrowser->OnDrop(urlList);
		}
		GlobalUnlock(medium.hGlobal);
	}
	TreeView_SelectDropTarget(m_hTargetWnd, NULL);
	return true;
}

HRESULT CTreeDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR *pdwEffect)
{
	TVHITTESTINFO hit;
	hit.pt = (POINT&)pt;
	ScreenToClient(m_hTargetWnd,&hit.pt);
	hit.flags = TVHT_ONITEM;
	HTREEITEM hItem = TreeView_HitTest(m_hTargetWnd,&hit);
	if (hItem != NULL)
	{
		TreeView_SelectDropTarget(m_hTargetWnd, hItem);
	}
	else
	{
		TreeView_SelectDropTarget(m_hTargetWnd, NULL);
		*pdwEffect = DROPEFFECT_NONE;
	}
	return CIDropTarget::DragOver(grfKeyState, pt, pdwEffect);
}

HRESULT CTreeDropTarget::DragLeave(void)
{
	TreeView_SelectDropTarget(m_hTargetWnd, NULL);
	return CIDropTarget::DragLeave();
}

CListDropTarget::CListDropTarget(CRepositoryBrowser * pRepoBrowser):CIDropTarget(pRepoBrowser->m_RepoList.GetSafeHwnd())
	, m_pRepoBrowser(pRepoBrowser)
{
}	

bool CListDropTarget::OnDrop(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD *pdwEffect)
{
	if(pFmtEtc->cfFormat == CF_UNICODETEXT && medium.tymed == TYMED_HGLOBAL)
	{
		TCHAR* pStr = (TCHAR*)GlobalLock(medium.hGlobal);
		CString urls;
		if(pStr != NULL)
		{
			urls = pStr;
		}
		GlobalUnlock(medium.hGlobal);
		urls.Replace(_T("\r\n"), _T("*"));
		CTSVNPathList urlList;
		urlList.LoadFromAsteriskSeparatedString(urls);
		m_pRepoBrowser->OnDrop(urlList);
	}

	if(pFmtEtc->cfFormat == CF_HDROP && medium.tymed == TYMED_HGLOBAL)
	{
		HDROP hDrop = (HDROP)GlobalLock(medium.hGlobal);
		if(hDrop != NULL)
		{
			CTSVNPathList urlList;
			TCHAR szFileName[MAX_PATH];

			UINT cFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0); 
			for(UINT i = 0; i < cFiles; ++i)
			{
				DragQueryFile(hDrop, i, szFileName, sizeof(szFileName));
				urlList.AddPath(CTSVNPath(szFileName));
			}
			m_pRepoBrowser->OnDrop(urlList);
		}
		GlobalUnlock(medium.hGlobal);
	}
	ListView_SetItemState(m_hTargetWnd, -1, 0, LVIS_DROPHILITED);
	return true; //let base free the medium
}

HRESULT CListDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR *pdwEffect)
{
	LVHITTESTINFO hit;
	hit.pt = (POINT&)pt;
	ScreenToClient(m_hTargetWnd,&hit.pt);
	hit.flags = LVHT_ONITEM;
	int iItem = ListView_HitTest(m_hTargetWnd,&hit);

	if (iItem >= 0)
	{
		CItem * pItem = (CItem*)m_pRepoBrowser->m_RepoList.GetItemData(iItem);
		if (pItem)
		{
			if (pItem->kind != svn_node_dir)
			{
				*pdwEffect = DROPEFFECT_NONE;
				ListView_SetItemState(m_hTargetWnd, -1, 0, LVIS_DROPHILITED);
			}
			else
			{
				ListView_SetItemState(m_hTargetWnd, -1, 0, LVIS_DROPHILITED);
				ListView_SetItemState(m_hTargetWnd, iItem, LVIS_DROPHILITED, LVIS_DROPHILITED);
			}
		}
	}
	return CIDropTarget::DragOver(grfKeyState, pt, pdwEffect);
}

HRESULT CListDropTarget::DragLeave(void)
{
	ListView_SetItemState(m_hTargetWnd, -1, 0, LVIS_DROPHILITED);
	return CIDropTarget::DragLeave();
}
