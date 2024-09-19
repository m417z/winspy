//
// DisplayDpiInfo.c
//
// Fills the DPI tab-pane with info for the current window.
//

#include "WinSpy.h"

#include "resource.h"
#include "Utils.h"


//
// Definitions from the platform SDK
//
#ifndef DPI_AWARENESS_CONTEXT_UNAWARE

DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);

#define DPI_AWARENESS_CONTEXT_UNAWARE               ((DPI_AWARENESS_CONTEXT)-1)
#define DPI_AWARENESS_CONTEXT_SYSTEM_AWARE          ((DPI_AWARENESS_CONTEXT)-2)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE     ((DPI_AWARENESS_CONTEXT)-3)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2  ((DPI_AWARENESS_CONTEXT)-4)
#define DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED     ((DPI_AWARENESS_CONTEXT)-5)

#define DPI_AWARENESS_SYSTEM_AWARE                  1

#endif



//
// These APIs exist on Windows 10 and later only.
//
typedef UINT (WINAPI * PFN_GetDpiForWindow)(HWND);
typedef DPI_AWARENESS_CONTEXT (WINAPI * PFN_GetWindowDpiAwarenessContext)(HWND);
typedef BOOL (WINAPI * PFN_AreDpiAwarenessContextsEqual)(DPI_AWARENESS_CONTEXT, DPI_AWARENESS_CONTEXT);
typedef BOOL (WINAPI * PFN_GetProcessDpiAwareness)(HANDLE, int *);
typedef DPI_AWARENESS_CONTEXT (WINAPI * PFN_GetDpiAwarenessContextForProcess)(HANDLE);
typedef UINT (WINAPI * PFN_GetSystemDpiForProcess)(HANDLE);
typedef BOOL (WINAPI * PFN_SetProcessDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);
typedef UINT (WINAPI * PFN_GetAwarenessFromDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);
typedef BOOL (WINAPI * PFN_GetWindowBand)(HWND hWnd, PDWORD pdwBand);

static PFN_GetDpiForWindow s_pfnGetDpiForWindow = NULL;
static PFN_GetWindowDpiAwarenessContext s_pfnGetWindowDpiAwarenessContext = NULL;
static PFN_AreDpiAwarenessContextsEqual s_pfnAreDpiAwarenessContextsEqual = NULL;
static PFN_GetProcessDpiAwareness s_pfnGetProcessDpiAwareness = NULL;
static PFN_GetDpiAwarenessContextForProcess s_pfnGetDpiAwarenessContextForProcess = NULL;
static PFN_GetSystemDpiForProcess s_pfnGetSystemDpiForProcess = NULL;
static PFN_SetProcessDpiAwarenessContext s_pfnSetProcessDpiAwarenessContext = NULL;
static PFN_GetAwarenessFromDpiAwarenessContext s_pfnGetAwarenessFromDpiAwarenessContext = NULL;
static PFN_GetWindowBand s_pfnGetWindowBand = NULL;

static BOOL s_fCheckedForAPIs = FALSE;

// https://blog.adeltax.com/window-z-order-in-windows-10/
static PCSTR s_pszWindowBands[] = {
     "ZBID_DEFAULT",
     "ZBID_DESKTOP",
     "ZBID_UIACCESS",
     "ZBID_IMMERSIVE_IHM",
     "ZBID_IMMERSIVE_NOTIFICATION",
     "ZBID_IMMERSIVE_APPCHROME",
     "ZBID_IMMERSIVE_MOGO",
     "ZBID_IMMERSIVE_EDGY",
     "ZBID_IMMERSIVE_INACTIVEMOBODY",
     "ZBID_IMMERSIVE_INACTIVEDOCK",
     "ZBID_IMMERSIVE_ACTIVEMOBODY",
     "ZBID_IMMERSIVE_ACTIVEDOCK",
     "ZBID_IMMERSIVE_BACKGROUND",
     "ZBID_IMMERSIVE_SEARCH",
     "ZBID_GENUINE_WINDOWS",
     "ZBID_IMMERSIVE_RESTRICTED",
     "ZBID_SYSTEM_TOOLS",

     //Windows 10+
     "ZBID_LOCK",
     "ZBID_ABOVELOCK_UX",
};

void InitializeDpiApis()
{
    if (!s_fCheckedForAPIs)
    {
        HMODULE hmod = GetModuleHandle(L"user32");

        s_pfnGetDpiForWindow = (PFN_GetDpiForWindow)GetProcAddress(hmod, "GetDpiForWindow");
        s_pfnGetWindowDpiAwarenessContext = (PFN_GetWindowDpiAwarenessContext)GetProcAddress(hmod, "GetWindowDpiAwarenessContext");
        s_pfnAreDpiAwarenessContextsEqual = (PFN_AreDpiAwarenessContextsEqual)GetProcAddress(hmod, "AreDpiAwarenessContextsEqual");
        s_pfnGetProcessDpiAwareness = (PFN_GetProcessDpiAwareness)GetProcAddress(hmod, "GetProcessDpiAwareness");
        s_pfnGetDpiAwarenessContextForProcess = (PFN_GetDpiAwarenessContextForProcess)GetProcAddress(hmod, "GetDpiAwarenessContextForProcess");
        s_pfnGetSystemDpiForProcess = (PFN_GetSystemDpiForProcess)GetProcAddress(hmod, "GetSystemDpiForProcess");
        s_pfnSetProcessDpiAwarenessContext = (PFN_SetProcessDpiAwarenessContext)GetProcAddress(hmod, "SetProcessDpiAwarenessContext");
        s_pfnGetAwarenessFromDpiAwarenessContext = (PFN_GetAwarenessFromDpiAwarenessContext)GetProcAddress(hmod, "GetAwarenessFromDpiAwarenessContext");
        s_pfnGetWindowBand = (PFN_GetWindowBand)GetProcAddress(hmod, "GetWindowBand");
        s_fCheckedForAPIs = TRUE;
    }
}

BOOL IsGetSystemDpiForProcessPresent()
{
    InitializeDpiApis();

    return (s_pfnGetSystemDpiForProcess != NULL);
}

void DescribeDpiAwarenessContext(DPI_AWARENESS_CONTEXT dpiContext, PSTR pszBuffer, size_t cchBuffer)
{
    PSTR pszValue = NULL;

    if (s_pfnAreDpiAwarenessContextsEqual(dpiContext, DPI_AWARENESS_CONTEXT_UNAWARE))
    {
        pszValue = "Unaware";
    }
    else if (s_pfnAreDpiAwarenessContextsEqual(dpiContext, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE))
    {
        pszValue = "Per-Monitor Aware";
    }
    else if (s_pfnAreDpiAwarenessContextsEqual(dpiContext, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
    {
        pszValue = "Per-Monitor Aware v2";
    }
    else if (s_pfnAreDpiAwarenessContextsEqual(dpiContext, DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED))
    {
        pszValue = "Unaware (GDI Scaled)";
    }
    else if (s_pfnGetAwarenessFromDpiAwarenessContext(dpiContext) == DPI_AWARENESS_SYSTEM_AWARE)
    {
        //
        // Windows 10 version 1803 (April 2018 Update) introduced a feature
        // where system-aware applications/windows can be associated with the
        // DPI of the primary monitor at the point in time that the process
        // started, rather than the older behavior where system-aware was
        // associated with the DPI of the primary monitor when the user
        // session started.
        //
        // This means that the system-aware DPI context from two different
        // processes may be associated with different DPI values, and that
        // means that we cannot use AreDpiAwarenessContextsEqual to determine
        // if the other process is system-aware.  Instead we can extract the
        // underlying awareness enum from the context and examine it instead.
        //

        pszValue = "System Aware";
    }

    if (pszValue)
    {
        StringCchCopyA(pszBuffer, cchBuffer, pszValue);
    }
    else
    {
        StringCchPrintfA(pszBuffer, cchBuffer, "Unknown (0x%08p)", dpiContext);
    }
}

void DescribeProcessDpiAwareness(DWORD dwProcessId, PSTR pszAwareness, size_t cchAwareness, PSTR pszDpi, size_t cchDpi)
{
    HANDLE hProcess = NULL;

    InitializeDpiApis();

    *pszAwareness = '\0';
    *pszDpi = '\0';

    if (s_pfnGetDpiAwarenessContextForProcess || s_pfnGetProcessDpiAwareness)
    {
        hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwProcessId);

        if (!hProcess)
        {
            DWORD dwError = GetLastError();

            if (dwError == ERROR_ACCESS_DENIED)
            {
                StringCchCopyA(pszAwareness, cchAwareness, "<Access Denied>");
                StringCchCopyA(pszDpi, cchDpi, "<Access Denied>");
            }
        }
    }
    else
    {
        StringCchCopyA(pszAwareness, cchAwareness, "<Unavailable>");
    }

    if (hProcess)
    {
        if (s_pfnGetDpiAwarenessContextForProcess)
        {
            DPI_AWARENESS_CONTEXT dpiContext = s_pfnGetDpiAwarenessContextForProcess(hProcess);

            if (dpiContext)
            {
                DescribeDpiAwarenessContext(dpiContext, pszAwareness, cchAwareness);
            }
        }
        else if (s_pfnGetProcessDpiAwareness)
        {
            CHAR  szValue[MAX_PATH] = "?";
            PCSTR pszValue = szValue;
            int dwLevel;

            if (s_pfnGetProcessDpiAwareness(hProcess, &dwLevel))
            {
                switch (dwLevel)
                {
                    case 0: // PROCESS_DPI_UNAWARE
                        pszValue = "Unaware";
                        break;

                    case 1: // PROCESS_SYSTEM_DPI_AWARE
                        pszValue = "System Aware";
                        break;

                    case 2: // PROCESS_PER_MONITOR_DPI_AWARE
                        pszValue = "Per-Monitor Aware";
                        break;

                    default:
                        sprintf_s(szValue, ARRAYSIZE(szValue), "Unknown (%d)", dwLevel);
                        break;
                }
            }

            StringCchCopyA(pszAwareness, cchAwareness, pszValue);
        }

        if (s_pfnGetSystemDpiForProcess)
        {
            UINT dpi = s_pfnGetSystemDpiForProcess(hProcess);
            UINT percent = (UINT)(dpi * 100 / 96);

            if (dpi)
            {
                sprintf_s(pszDpi, cchDpi, "%d  (%u%%)", dpi, percent);
            }
            else
            {
                StringCchCopyA(pszDpi, cchDpi, "<Unavailable>");
            }
        }

        CloseHandle(hProcess);
    }
}

//
// Update the DPI tab for the specified window
//
void UpdateDpiTab(HWND hwnd)
{
    HWND hwndDlg = WinSpyTab[DPI_TAB].hwnd;
    CHAR szTemp[100];
    PSTR pszValue = NULL;
    BOOL fValid;

    InitializeDpiApis();

    fValid = (hwnd && IsWindow(hwnd));

    if (!fValid)
    {
        pszValue = (hwnd == NULL) ? "" : "(invalid window)";
    }

    // DPI field

    if (fValid)
    {
        if (s_pfnGetDpiForWindow)
        {
            UINT dpi     = s_pfnGetDpiForWindow(hwnd);
            UINT percent = (UINT)(dpi * 100 / 96);

            sprintf_s(szTemp, ARRAYSIZE(szTemp), "%d  (%u%%)", dpi, percent);
            pszValue = szTemp;
        }
        else
        {
            pszValue = "<Unavailable>";
        }
    }

    SetDlgItemTextExA(hwndDlg, IDC_WINDOW_DPI, pszValue);

    // DPI awareness field

    if (fValid)
    {
        if (s_pfnGetWindowDpiAwarenessContext)
        {
            DPI_AWARENESS_CONTEXT dpiContext = s_pfnGetWindowDpiAwarenessContext(hwnd);

            DescribeDpiAwarenessContext(dpiContext, szTemp, ARRAYSIZE(szTemp));
            pszValue = szTemp;
        }
        else
        {
            pszValue = "<Unavailable>";
        }
    }

    SetDlgItemTextExA(hwndDlg, IDC_WINDOW_DPI_AWARENESS, pszValue);

    // Band field

    if (fValid)
    {
        if (s_pfnGetWindowBand)
        {
            DWORD dwBand = 0;
            s_pfnGetWindowBand(hwnd, &dwBand);

            sprintf_s(szTemp, ARRAYSIZE(szTemp), "%s  (%u)",
                dwBand < ARRAYSIZE(s_pszWindowBands) ? s_pszWindowBands[dwBand] : "Unknown", dwBand);
            pszValue = szTemp;
        }
        else
        {
            pszValue = "<Unavailable>";
        }
    }

    SetDlgItemTextExA(hwndDlg, IDC_WINDOW_BAND, pszValue);

}

void MarkProcessAsPerMonitorDpiAware()
{
    InitializeDpiApis();

    if (s_pfnSetProcessDpiAwarenessContext)
    {
        s_pfnSetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
    }
}

void MarkProcessAsSystemDpiAware()
{
    InitializeDpiApis();

    if (s_pfnSetProcessDpiAwarenessContext)
    {
        s_pfnSetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
    }
}

UINT g_SystemDPI = 0;

int DPIScale(HWND hwnd, int value)
{
    // If the system supports it (Windows 10+) then we use GetDpiForWindow
    // to determine the DPI associated with the window.  Otherwise, we will
    // query the 'system' DPI that the winspy process is running under via
    // GetDeviceCaps+LOGPIXELSX on a screen DC.

    if (g_SystemDPI == 0)
    {
        InitializeDpiApis();

        HDC hdc = GetDC(NULL);
        g_SystemDPI = GetDeviceCaps(hdc, LOGPIXELSX);
        DeleteDC(hdc);
    }

    int dpi = 0;

    if (s_pfnGetDpiForWindow)
    {
        dpi = s_pfnGetDpiForWindow(hwnd);
    }
    else
    {
        dpi = g_SystemDPI;
    }

    return MulDiv(value, dpi, 96);
}
