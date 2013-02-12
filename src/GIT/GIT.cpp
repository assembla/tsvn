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
#include "stdafx.h"
#include "GIT.h"
#include "TortoiseProc.h"
#include "SimplePrompt.h"
#include "UnicodeUtils.h"



GIT::GIT()
    : m_nCmdState(0)
{
    git_threads_init();
}


GIT::~GIT()
{
}


BOOL GIT::SimplePrompt(CString& username, CString& password, const CString& Realm)
{
    CSimplePrompt dlg(CWnd::FromHandle(FindParentWindow(NULL)));
    dlg.m_sRealm = Realm;
    INT_PTR nResponse = dlg.DoModal();
    if (nResponse == IDOK)
    {
        username = dlg.m_sUsername;
        password = dlg.m_sPassword;
        return TRUE;
    }
    return FALSE;
}

int GIT::cred_acquire(git_cred **out,
                      const char * url,
                      const char * username_from_url,
                      unsigned int /*allowed_types*/,
                      void * payload)
{
    GIT * pGit = (GIT*)payload;
    CString username = CUnicodeUtils::GetUnicode(username_from_url);
    CString password;
    CString realm = CUnicodeUtils::GetUnicode(url);
    if (pGit->SimplePrompt(username, password, realm))
    {
        return git_cred_userpass_plaintext_new(out, CUnicodeUtils::GetUTF8(username), CUnicodeUtils::GetUTF8(password));
    }
    return 1;
}

int GIT::checkout_notify(git_checkout_notify_t why,
                         const char *path,
                         const git_diff_file *baseline,
                         const git_diff_file *target,
                         const git_diff_file *workdir,
                         void *payload)
{
    GIT * pGit = (GIT*)payload;
    return pGit->CheckoutNotify(why, CUnicodeUtils::GetUnicode(path));
}

void GIT::fetch_progress(const git_transfer_progress *stats, void *payload)
{
    GIT * pGit = (GIT*)payload;
    if (pGit->m_nCmdState == 0)
    {
        pGit->m_nCmdState = 1;
        auto t = pGit->m_progressInfos.front();
        pGit->GitNotify(std::get<0>(t), std::get<1>(t));
        pGit->m_progressInfos.pop_front();
    }
    pGit->progress_func(stats->received_bytes, stats->received_objects, stats->total_objects, payload);
}
void GIT::checkout_progress(const char * /*path*/, size_t cur, size_t tot, void *payload)
{
    GIT * pGit = (GIT*)payload;
    if (pGit->m_nCmdState == 1)
    {
        pGit->m_nCmdState = 2;
        auto t = pGit->m_progressInfos.front();
        pGit->GitNotify(std::get<0>(t), std::get<1>(t));
        pGit->m_progressInfos.pop_front();
    }
    pGit->progress_func(0, cur, tot, payload);
}

void GIT::progress_func(size_t recbytes, size_t progress, size_t total, void *baton)
{
    GIT * pGit = (GIT*)baton;
    if ((pGit==0)||((pGit->m_progressWnd == 0)&&(pGit->m_pProgressDlg == 0)))
        return;
    apr_off_t delta = recbytes;
    if ((recbytes >= pGit->m_progress_lastprogress)&&(total == pGit->m_progress_lasttotal))
        delta = recbytes - pGit->m_progress_lastprogress;

    pGit->m_progress_lastprogress = recbytes;
    pGit->m_progress_lasttotal = total;

    DWORD ticks = GetTickCount();
    pGit->m_progress_sum += delta;
    pGit->m_progress_total += delta;
    if ((pGit->m_progress_lastTicks + 1000UL) < ticks)
    {
        double divby = (double(ticks - pGit->m_progress_lastTicks)/1000.0);
        if (divby < 0.0001)
            divby = 1;
        pGit->m_GITProgressMSG.overall_total = pGit->m_progress_total;
        pGit->m_GITProgressMSG.progress = progress;
        pGit->m_GITProgressMSG.total = total;
        pGit->m_progress_lastTicks = ticks;
        size_t average = size_t(double(pGit->m_progress_sum) / divby);
        pGit->m_progress_sum = 0;

        pGit->m_GITProgressMSG.BytesPerSecond = average;
        if (average < 1024LL)
            pGit->m_GITProgressMSG.SpeedString.Format(IDS_SVN_PROGRESS_BYTES_SEC, average);
        else
        {
            double averagekb = (double)average / 1024.0;
            pGit->m_GITProgressMSG.SpeedString.Format(IDS_SVN_PROGRESS_KBYTES_SEC, averagekb);
        }
        CString s;
        s.Format(IDS_SVN_PROGRESS_SPEED, (LPCWSTR)pGit->m_GITProgressMSG.SpeedString);
        pGit->m_GITProgressMSG.SpeedString = s;
        if (pGit->m_progressWnd)
            PostMessage(pGit->m_progressWnd, WM_SVNPROGRESS, 0, (LPARAM)&pGit->m_GITProgressMSG);
        else if (pGit->m_pProgressDlg)
        {
            if ((pGit->m_bShowProgressBar && (progress > 1000LL) && (total > 0LL)))
                pGit->m_pProgressDlg->SetProgress64(progress, total);
            pGit->m_pProgressDlg->SetLine(2, pGit->m_GITProgressMSG.SpeedString);
        }
    }
    return;
}

void GIT::SetAndClearProgressInfo(HWND hWnd)
{
    m_progressWnd = hWnd;
    m_pProgressDlg = NULL;
    m_progress_total = 0;
    m_progress_sum = 0;
    m_progress_lastprogress = 0;
    m_progress_lasttotal = 0;
    m_progress_lastTicks = GetTickCount();
}

void GIT::SetAndClearProgressInfo(CProgressDlg * pProgressDlg, bool bShowProgressBar/* = false*/)
{
    m_progressWnd = NULL;
    m_pProgressDlg = pProgressDlg;
    m_progress_total = 0;
    m_progress_sum = 0;
    m_progress_lastprogress = 0;
    m_progress_lasttotal = 0;
    m_progress_lastTicks = GetTickCount();
    m_bShowProgressBar = bShowProgressBar;
}

bool GIT::Clone( const CString& url, const CString& local )
{
    Prepare();
    git_repository *cloned_repo = NULL;
    git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;
    git_checkout_opts checkout_opts = GIT_CHECKOUT_OPTS_INIT;

    checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE_CREATE;
    checkout_opts.progress_cb = checkout_progress;
    checkout_opts.progress_payload = this;
    checkout_opts.notify_cb = checkout_notify;
    checkout_opts.notify_payload = this;
    checkout_opts.notify_flags = GIT_CHECKOUT_NOTIFY_CONFLICT|GIT_CHECKOUT_NOTIFY_DIRTY|GIT_CHECKOUT_NOTIFY_UPDATED;
    clone_opts.checkout_opts = checkout_opts;
    clone_opts.fetch_progress_cb = &fetch_progress;
    clone_opts.fetch_progress_payload = this;
    clone_opts.cred_acquire_cb = cred_acquire;
    clone_opts.cred_acquire_payload = this;

    CString sTmp;
    sTmp.Format(IDS_PROGRS_CMD_CLONE4, (LPCWSTR)url);
    m_progressInfos.push_back(std::make_tuple(L"", sTmp));
    sTmp.Format(IDS_PROGRS_CMD_CLONE2, local);
    m_progressInfos.push_back(std::make_tuple(L"", sTmp));

    int error = git_clone(&cloned_repo, CUnicodeUtils::GetUTF8(url), CUnicodeUtils::GetUTF8(local), &clone_opts);
    if (error != 0)
    {
        const git_error *err = giterr_last();
        if (err)
        {
            m_sError.Format(L"Error: %d\n%s", err->klass, (LPCWSTR)CUnicodeUtils::GetUnicode(err->message));
        }
        else
            m_sError.Empty();
    }
    else if (cloned_repo)
    {
        git_repository_free(cloned_repo);
    }

    return error == 0;
}

void GIT::Prepare()
{
    m_sError.Empty();
    m_nCmdState = 0;
}
