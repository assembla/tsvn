#pragma once
#include "ResizableDialog.h"
#include "RevisionGraph.h"
#include "ProgressDlg.h"


class CRevisionGraphDlg : public CResizableDialog, public CRevisionGraph
{
	DECLARE_DYNAMIC(CRevisionGraphDlg)

public:
	CRevisionGraphDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CRevisionGraphDlg();

	virtual BOOL OnInitDialog();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnPaint();

// Dialog Data
	enum { IDD = IDD_REVISIONGRAPH };

	CString			m_sPath;
	CProgressDlg	m_Progress;
protected:
	HICON			m_hIcon;
	HANDLE			m_hThread;
	DWORD			m_dwTicks;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL ProgressCallback(CString text, CString text2, DWORD done, DWORD total);

	DECLARE_MESSAGE_MAP()
public:
};
DWORD WINAPI WorkerThread(LPVOID pVoid);
