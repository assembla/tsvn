// RevisionGrapgDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include ".\revisiongraphdlg.h"


// CRevisionGraphDlg dialog

IMPLEMENT_DYNAMIC(CRevisionGraphDlg, CResizableDialog)
CRevisionGraphDlg::CRevisionGraphDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CRevisionGraphDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CRevisionGraphDlg::~CRevisionGraphDlg()
{
}

void CRevisionGraphDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
}

void CRevisionGraphDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CResizableDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CRevisionGraphDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BEGIN_MESSAGE_MAP(CRevisionGraphDlg, CResizableDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()


// CRevisionGraphDlg message handlers

BOOL CRevisionGraphDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();

	CString temp;
	temp.LoadString(IDS_REVGRAPH_PROGTITLE);
	m_Progress.SetTitle(temp);
	temp.LoadString(IDS_REVGRAPH_PROGCANCEL);
	m_Progress.SetCancelMsg(temp);
	m_Progress.SetTime();

	m_dwTicks = GetTickCount();
	DWORD dwThreadId;
	if ((m_hThread = CreateThread(NULL, 0, &WorkerThread, this, 0, &dwThreadId))==0)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CRevisionGraphDlg::ProgressCallback(CString text, CString text2, DWORD done, DWORD total)
{
	if ((m_dwTicks+100)<GetTickCount())
	{
		m_dwTicks = GetTickCount();
		m_Progress.SetLine(1, text);
		m_Progress.SetLine(2, text2);
		m_Progress.SetProgress(done, total);
		if (m_Progress.HasUserCancelled())
			return FALSE;
	}
	return TRUE;
}

DWORD WINAPI WorkerThread(LPVOID pVoid)
{
	//get the status of all selected file/folders recursively
	//and show the ones which have to be committed to the user
	//in a listcontrol. 
	CRevisionGraphDlg*	pDlg;
	pDlg = (CRevisionGraphDlg*)pVoid;
	pDlg->m_Progress.ShowModeless(pDlg->m_hWnd);
	pDlg->FetchRevisionData(pDlg->m_sPath);
	pDlg->AnalyzeRevisionData(pDlg->m_sPath);
	pDlg->m_Progress.Stop();
	return 0;
}