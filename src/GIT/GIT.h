// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2013 - TortoiseSVN

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

#pragma once
#include "SVN.h"
#include "ProgressDlg.h"
#include "git2.h"

class GIT
{
public:
    GIT();
    ~GIT();

    // overrides
    virtual int CheckoutNotify(git_checkout_notify_t /*why*/, const CString& /*path*/) { return 0; }
    virtual void GitNotify(const CString& /*action*/, const CString& /*info*/) {}

    bool Clone(const CString& url, const CString& local);

    CString GetLastError() { return m_sError; }
    void SetAndClearProgressInfo(HWND hWnd);
    void SetAndClearProgressInfo(CProgressDlg * pProgressDlg, bool bShowProgressBar = false);

protected:
    void Prepare();

    BOOL SimplePrompt(CString& username, CString& password, const CString& Realm);
    void progress_func(size_t recbytes, size_t progress, size_t total, void *baton);
    static int checkout_notify(git_checkout_notify_t why,
                               const char *path,
                               const git_diff_file *baseline,
                               const git_diff_file *target,
                               const git_diff_file *workdir,
                               void *payload);
    static void checkout_progress(const char *path, size_t cur, size_t tot, void *payload);
    static void fetch_progress(const git_transfer_progress *stats, void *payload);
    static int cred_acquire(git_cred **out,
                            const char * url,
                            const char * username_from_url,
                            unsigned int allowed_types,
                            void * payload);

private:
    CString         m_sError;
    int             m_nCmdState;
    SVN::SVNProgress m_GITProgressMSG;
    HWND            m_progressWnd;
    CProgressDlg *  m_pProgressDlg;
    bool            m_progressWndIsCProgress;
    bool            m_bShowProgressBar;
    size_t          m_progress_total;
    size_t          m_progress_sum;
    size_t          m_progress_averagehelper;
    size_t          m_progress_lastprogress;
    size_t          m_progress_lasttotal;
    DWORD           m_progress_lastTicks;
    std::deque<std::tuple<CString, CString>> m_progressInfos;
};


