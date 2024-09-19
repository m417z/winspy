//
//  DisplayProcessInfo.c
//  Copyright (c) 2002 by J Brown
//  Freeware
//
//  void SetProcesInfo(HWND hwnd)
//
//  Fill the process-tab-pane with process info for the
//  specified window.
//

#include "WinSpy.h"

#include "Utils.h"
#include <shellapi.h>
#include <psapi.h>
#include "resource.h"
#include <tlhelp32.h>

void DescribeProcessDpiAwareness(DWORD dwProcessId, PSTR pszAwareness, size_t cchAwareness, PSTR pszDpi, size_t cchDpi);
BOOL IsGetSystemDpiForProcessPresent();

typedef BOOL(WINAPI * EnumProcessModulesProc)(HANDLE, HMODULE *, DWORD, PDWORD);
typedef DWORD(WINAPI * GetModuleBaseNameProc)(HANDLE, HMODULE, PWSTR, DWORD);
typedef DWORD(WINAPI * GetModuleFileNameExProc)(HANDLE, HMODULE, PWSTR, DWORD);

typedef BOOL(WINAPI * QueryFullProcessImageNameProc)(HANDLE hProcess, DWORD dwFlags, PWSTR lpExeName, PDWORD lpdwSize);

typedef HRESULT(WINAPI * GetThreadDescriptionProc)(HANDLE hThread, PWSTR *ppszThreadDescription);

HRESULT TryGetThreadDescription(HANDLE hThread, PWSTR *ppszThreadDescription) {
    static GetThreadDescriptionProc fnGetThreadDescription = NULL;

    if (!fnGetThreadDescription)
    {
        fnGetThreadDescription = (GetThreadDescriptionProc)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "GetThreadDescription");

        if (!fnGetThreadDescription)
            return E_FAIL;
    }

    return fnGetThreadDescription(hThread, ppszThreadDescription);
}

BOOL GetProcessNameByPid1(DWORD dwProcessId, WCHAR szName[], DWORD nNameSize, WCHAR szPath[], DWORD nPathSize)
{
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe = { sizeof(pe) };
    BOOL fFound = FALSE;

    szPath[0] = '\0';
    szName[0] = '\0';

    if (Process32First(h, &pe))
    {
        do
        {
            if (pe.th32ProcessID == dwProcessId)
            {
                if (szName)
                {
                    lstrcpyn(szName, pe.szExeFile, nNameSize);
                }

                if (szPath)
                {
                    //OpenProcess(
                    lstrcpyn(szPath, pe.szExeFile, nPathSize);
                }

                fFound = TRUE;
                break;
            }
        } while (Process32Next(h, &pe));
    }

    CloseHandle(h);

    return fFound;
}


//
// This uses PSAPI.DLL, which is only available under NT/2000/XP I think,
// so we dynamically load this library, so that we can still run under 9x.
//
//  dwProcessId  [in]
//  szName       [out]
//  nNameSize    [in]
//  szPath       [out]
//  nPathSize    [in]
//
BOOL GetProcessNameByPid_BelowVista(DWORD dwProcessId, WCHAR szName[], DWORD nNameSize, WCHAR szPath[], DWORD nPathSize)
{
    HMODULE hPSAPI;
    HANDLE hProcess;

    HMODULE hModule;
    DWORD   dwNumModules;

    EnumProcessModulesProc  fnEnumProcessModules;
    GetModuleBaseNameProc   fnGetModuleBaseName;
    GetModuleFileNameExProc fnGetModuleFileNameEx;

    // Attempt to load Process Helper library
    hPSAPI = LoadLibrary(L"psapi.dll");

    if (!hPSAPI)
    {
        szName[0] = '\0';
        return FALSE;
    }

    // OK, we have access to the PSAPI functions, so open the process
    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessId);
    if (!hProcess)
    {
        FreeLibrary(hPSAPI);
        return FALSE;
    }


    fnEnumProcessModules = (EnumProcessModulesProc)GetProcAddress(hPSAPI, "EnumProcessModules");
    fnGetModuleBaseName = (GetModuleBaseNameProc)GetProcAddress(hPSAPI, "GetModuleBaseNameW");
    fnGetModuleFileNameEx = (GetModuleFileNameExProc)GetProcAddress(hPSAPI, "GetModuleFileNameExW");

    if (!fnEnumProcessModules || !fnGetModuleBaseName)
    {
        CloseHandle(hProcess);
        FreeLibrary(hPSAPI);
        return FALSE;
    }

    // Find the first module
    if (fnEnumProcessModules(hProcess, &hModule, sizeof(hModule), &dwNumModules))
    {
        // Now get the module name
        if (szName)
            fnGetModuleBaseName(hProcess, hModule, szName, nNameSize);

        // get module filename
        if (szPath)
            fnGetModuleFileNameEx(hProcess, hModule, szPath, nPathSize);
    }
    else
    {
        CloseHandle(hProcess);
        FreeLibrary(hPSAPI);
        return FALSE;
    }

    CloseHandle(hProcess);
    FreeLibrary(hPSAPI);

    return TRUE;
}

BOOL GetProcessNameByPid(DWORD dwProcessId, WCHAR szName[], DWORD nNameSize, WCHAR szPath[], DWORD nPathSize)
{
    static QueryFullProcessImageNameProc fnQueryFullProcessImageName = NULL;
    HANDLE hProcess;
    DWORD dwSize;
    WCHAR *pName;
    BOOL bSucceeded;

    if (!fnQueryFullProcessImageName)
    {
#ifdef UNICODE
        fnQueryFullProcessImageName = (QueryFullProcessImageNameProc)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "QueryFullProcessImageNameW");
#else
        fnQueryFullProcessImageName = (QueryFullProcessImageNameProc)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "QueryFullProcessImageNameA");
#endif

        if (!fnQueryFullProcessImageName)
            return GetProcessNameByPid_BelowVista(dwProcessId, szName, nNameSize, szPath, nPathSize);
    }

    bSucceeded = FALSE;

    hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwProcessId);
    if (hProcess)
    {
        dwSize = nPathSize;

        if (fnQueryFullProcessImageName(hProcess, 0, szPath, &dwSize))
        {
            pName = wcsrchr(szPath, '\\');
            if (pName)
            {
                wcsncpy_s(szName, nNameSize, pName + 1, _TRUNCATE);
                bSucceeded = TRUE;
            }
        }

        CloseHandle(hProcess);
    }

    return bSucceeded;
}


//
//  Update the Process tab for the specified window
//
void UpdateProcessTab(HWND hwnd, DWORD dwOverridePID)
{
    DWORD dwProcessId = 0;
    DWORD dwThreadId = 0;
    WCHAR ach[32];
    WCHAR szPath[MAX_PATH];
    BOOL  fValid;
    HWND  hwndDlg = WinSpyTab[PROCESS_TAB].hwnd;
    PCWSTR pszDefault = L"";

    if (hwnd)
    {
        if (IsWindow(hwnd))
        {
            fValid = TRUE;
            dwThreadId = GetWindowThreadProcessId(hwnd, &dwProcessId);
        }
        else
        {
            fValid = FALSE;
            pszDefault = szInvalidWindow;
        }
    }
    else
    {
        fValid = FALSE;
        dwProcessId = dwOverridePID;
    }

    // Process Id

    if (dwProcessId)
    {
        FormatDlgItemText(hwndDlg, IDC_PID, L"%08X  (%u)", dwProcessId, dwProcessId);
    }
    else
    {
        SetDlgItemTextEx(hwndDlg, IDC_PID, pszDefault);
    }


    // Thread Id

    if (fValid)
    {
        PWSTR pszThreadDescription = NULL;
        HANDLE hThread = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, dwThreadId);
        if (hThread) {
            if (FAILED(TryGetThreadDescription(hThread, &pszThreadDescription))) {
                pszThreadDescription = NULL;
            }

            CloseHandle(hThread);
        }

        FormatDlgItemText(hwndDlg, IDC_TID, L"%08X  (%u)%s%s", dwThreadId, dwThreadId,
            (pszThreadDescription && *pszThreadDescription) ? L"  -  " : L"", pszThreadDescription ? pszThreadDescription : L"");

        if (pszThreadDescription) {
            LocalFree(pszThreadDescription);
        }
    }
    else
    {
        SetDlgItemTextEx(hwndDlg, IDC_TID, pszDefault);
    }


    // Try to get process name and path
    if (dwProcessId && GetProcessNameByPid(dwProcessId, ach, ARRAYSIZE(ach),
        szPath, ARRAYSIZE(szPath)))
    {
        SetDlgItemTextEx(hwndDlg, IDC_PROCESSNAME, ach);
        SetDlgItemTextEx(hwndDlg, IDC_PROCESSPATH, szPath);
    }
    else
    {
        SetDlgItemTextEx(hwndDlg, IDC_PROCESSNAME, fValid ? L"N/A" : pszDefault);
        SetDlgItemTextEx(hwndDlg, IDC_PROCESSPATH, fValid ? L"N/A" : pszDefault);
    }

    if (dwProcessId)
    {
        CHAR szMode[100];
        CHAR szDpi[100];

        DescribeProcessDpiAwareness(dwProcessId, szMode, ARRAYSIZE(szMode), szDpi, ARRAYSIZE(szDpi));

        SetDlgItemTextExA(hwndDlg, IDC_PROCESS_DPI_AWARENESS, szMode);
        SetDlgItemTextExA(hwndDlg, IDC_PROCESS_SYSTEM_DPI, szDpi);

        if (!IsGetSystemDpiForProcessPresent())
        {
            ShowWindow(GetDlgItem(hwndDlg, IDC_PROCESS_SYSTEM_DPI_LABEL), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_PROCESS_SYSTEM_DPI), SW_HIDE);
        }
    }
    else
    {
        SetDlgItemTextEx(hwndDlg, IDC_PROCESS_DPI_AWARENESS, pszDefault);
        SetDlgItemTextEx(hwndDlg, IDC_PROCESS_SYSTEM_DPI, pszDefault);
    }
}


WCHAR szWarning1[] = L"Are you sure you want to close this process?";
WCHAR szWarning2[] = L"WARNING: Terminating a process can cause undesired\r\n"\
L"results including loss of data and system instability. The\r\n"\
L"process will not be given the chance to save its state or\r\n"\
L"data before it is terminated. Are you sure you want to\r\n"\
L"terminate the process?";

void ShowProcessContextMenu(HWND hwndParent, INT x, INT y, BOOL fForButton, HWND hwnd, DWORD dwProcessId)
{
    DWORD dwThreadId = 0;
    HMENU hMenu, hPopup;
    UINT  uCmd;
    DWORD dwFlags;

    hMenu  = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENU_PROCESS));
    hPopup = GetSubMenu(hMenu, 0);

    if (hwnd)
    {
        dwProcessId = 0;
        (void)GetWindowThreadProcessId(hwnd, &dwProcessId);
    }
    else
    {
        // If there is no current window, then we are working off a process
        // node selected in the window tree.  The 'End Process (safe)' option
        // is currently built around posting WM_QUIT to the current window's
        // thread.  For now, we just disable the option because we don't
        // know what thread to target.  This could be enabled by enumerating
        // the windows in the process to pick a suitable thread.

        EnableMenuItem(hPopup, IDM_WINSPY_POSTQUIT, MF_BYCOMMAND | MF_GRAYED);
    }

    if (fForButton)
    {
        dwFlags = TPM_RIGHTALIGN | TPM_TOPALIGN | TPM_RETURNCMD;
    }
    else
    {
        dwFlags = TPM_RIGHTBUTTON | TPM_RETURNCMD;
    }

    uCmd = TrackPopupMenu(hPopup, dwFlags, x, y, 0, hwndParent, 0);

    switch (uCmd)
    {
        case IDM_WINSPY_FINDEXE:
        {
            WCHAR szExplorer[MAX_PATH];
            WCHAR szName[32];
            WCHAR szPath[MAX_PATH];

            if (GetProcessNameByPid(dwProcessId, szName, ARRAYSIZE(szName), szPath, ARRAYSIZE(szPath)))
            {
                swprintf_s(szExplorer, ARRAYSIZE(szExplorer), L"/select,\"%s\"", szPath);
                ShellExecute(0, L"open", L"explorer", szExplorer, 0, SW_SHOW);
            }
            else
            {
                MessageBox(hwndParent, L"Invalid Process Id", szAppName, MB_OK | MB_ICONWARNING);
            }
        }
        break;

        // Forcibly terminate!
        case IDM_WINSPY_TERMINATE:
        {
            if (MessageBox(hwndParent, szWarning2, szAppName, MB_YESNO | MB_ICONWARNING) == IDYES)
            {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, dwProcessId);

                if (hProcess)
                {
                    TerminateProcess(hProcess, (UINT)-1);
                    CloseHandle(hProcess);
                }
                else
                {
                    MessageBox(hwndParent, L"Invalid Process Id", szAppName, MB_OK | MB_ICONWARNING);
                }
            }

            break;
        }

        // Cleanly exit. Won't work if app. is hung
        case IDM_WINSPY_POSTQUIT:
        {
            if (MessageBox(hwndParent, szWarning1, szAppName, MB_YESNO | MB_ICONWARNING) == IDYES)
            {
                PostThreadMessage(dwThreadId, WM_QUIT, 0, 0);
            }
            break;
        }
    }

    DestroyMenu(hMenu);
}
