//
//  Utils.c
//
//  Copyright (c) 2002 by J Brown
//  Freeware
//
//  Lots of utility and general helper functions.
//

#include "WinSpy.h"
#include <malloc.h>
#include "Utils.h"


//
// Case sensitive prefix match.
//

BOOL StrBeginsWith(PCWSTR pcsz, PCWSTR pcszPrefix)
{
    size_t cch = wcslen(pcszPrefix);

    return (wcsncmp(pcsz, pcszPrefix, cch) == 0);
}

//
//  Enable/Disable privilege with specified name (for current process)
//
BOOL EnablePrivilege(WCHAR *szPrivName, BOOL fEnable)
{
    TOKEN_PRIVILEGES tp;
    LUID    luid;
    HANDLE  hToken;

    if (!LookupPrivilegeValue(NULL, szPrivName, &luid))
        return FALSE;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return FALSE;

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = fEnable ? SE_PRIVILEGE_ENABLED : 0;

    AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);

    CloseHandle(hToken);

    return (GetLastError() == ERROR_SUCCESS);
}


BOOL EnableDebugPrivilege()
{
    return EnablePrivilege(SE_DEBUG_NAME, TRUE);
}


//
// Style helper functions
//
UINT AddStyle(HWND hwnd, UINT style)
{
    UINT oldstyle = GetWindowLong(hwnd, GWL_STYLE);
    SetWindowLong(hwnd, GWL_STYLE, oldstyle | style);
    return oldstyle;
}

UINT AddDlgItemStyle(HWND hwnd, UINT nCtrlId, UINT style)
{
    return AddStyle(GetDlgItem(hwnd, nCtrlId), style);
}

UINT DelStyle(HWND hwnd, UINT style)
{
    UINT oldstyle = GetWindowLong(hwnd, GWL_STYLE);
    SetWindowLong(hwnd, GWL_STYLE, oldstyle & ~style);
    return oldstyle;
}

UINT DelDlgItemStyle(HWND hwnd, UINT nCtrlId, UINT style)
{
    return DelStyle(GetDlgItem(hwnd, nCtrlId), style);
}

BOOL EnableDlgItem(HWND hwnd, UINT nCtrlId, BOOL fEnabled)
{
    return EnableWindow(GetDlgItem(hwnd, nCtrlId), fEnabled);
}

BOOL ShowDlgItem(HWND hwnd, UINT nCtrlId, DWORD dwShowCmd)
{
    return ShowWindow(GetDlgItem(hwnd, nCtrlId), dwShowCmd);
}


//
// SetDlgItemTextEx is a Variant of SetDlgItemText that avoids re-setting
// the value if it is the same as what is already in the control.  This allows
// you to, say, select the text in a label and not have the selection state
// cleared by a redundant reset of the same value.
//

void SetDlgItemTextEx(HWND hwndDlg, UINT id, PCWSTR pcszNew)
{
    HWND hwnd = GetDlgItem(hwndDlg, id);
    WCHAR szOld[256];

    if (hwnd)
    {
        GetWindowText(hwnd, szOld, ARRAYSIZE(szOld));

        if (wcscmp(pcszNew, szOld) != 0)
        {
            SetWindowText(hwnd, pcszNew);
        }
    }
}

void SetDlgItemTextExA(HWND hwndDlg, UINT id, PCSTR pcszNew)
{
    HWND hwnd = GetDlgItem(hwndDlg, id);
    CHAR szOld[256];

    if (hwnd)
    {
        GetWindowTextA(hwnd, szOld, ARRAYSIZE(szOld));

        if (strcmp(pcszNew, szOld) != 0)
        {
            SetWindowTextA(hwnd, pcszNew);
        }
    }
}

void FormatDlgItemText(HWND hwndDlg, UINT id, _Printf_format_string_ PCWSTR pcszFormat, ...)
{
    WCHAR szBuffer[256];

    va_list args;
    va_start(args, pcszFormat);
    StringCchVPrintfW(szBuffer, sizeof(szBuffer), pcszFormat, args);
    va_end(args);

    SetDlgItemTextEx(hwndDlg, id, szBuffer);
}

int WINAPI GetRectHeight(RECT *rect)
{
    return rect->bottom - rect->top;
}

int WINAPI GetRectWidth(RECT *rect)
{
    return rect->right - rect->left;
}

//
// Return the window position of a control, in coordinates relative to the
// client area of the parent window.
//

RECT GetControlRect(HWND hwndParent, HWND hwnd)
{
    RECT rc;
    GetWindowRect(hwnd, &rc);

    ScreenToClient(hwndParent, (POINT *)&rc.left);
    ScreenToClient(hwndParent, (POINT *)&rc.right);

    return rc;
}

void SetControlRect(HWND hwnd, RECT* prc)
{
    SetWindowPos(
        hwnd,
        NULL,
        prc->left,
        prc->top,
        prc->right - prc->left,
        prc->bottom - prc->top,
        SWP_NOACTIVATE | SWP_NOZORDER);
}

//
//  Convert the specified string
//  into the equivalent decimal value
//
DWORD_PTR _tstrtoib10(WCHAR *szHexStr)
{
#ifdef _WIN64
    return _wtoi64(szHexStr);
#else
    return _wtoi(szHexStr);
#endif
}

//
//  Convert the specified string (with a hex-number in it)
//  into the equivalent hex-value
//
DWORD_PTR _tstrtoib16(PCWSTR pszHexStr)
{
    DWORD_PTR num = 0;
    PCWSTR pch = pszHexStr;

    // Skip any leading whitespace
    while (*pch == ' ')
        pch++;

    // Skip a "0x" prefix if present.
    if (*pch == '0' && *(pch + 1) == 'x')
        pch += 2;

    while (isxdigit(*pch))
    {
        static_assert('9' < 'A' && 'A' < 'a', "Incorrect character code values assumption");

        UINT ch = *pch;
        UINT x = ch <= '9' ?
            ch - '0' :
            10 + (ch < 'a' ?
                ch - 'A' :
                ch - 'a');

        num = (num << 4) | (x & 0x0f);
        pch++;
    }

    return num;
}

DWORD_PTR GetNumericValue(HWND hwnd, int base)
{
    WCHAR szAddressText[128];

    GetWindowText(hwnd, szAddressText, ARRAYSIZE(szAddressText));

    switch (base)
    {
    case 1:
    case 16:            //base is currently hex
        return _tstrtoib16(szAddressText);

    case 0:
    case 10:            //base is currently decimal
        return _tstrtoib10(szAddressText);

    default:
        return 0;
    }
}

DWORD_PTR GetDlgItemBaseInt(HWND hwnd, UINT ctrlid, int base)
{
    return GetNumericValue(GetDlgItem(hwnd, ctrlid), base);
}

//
//  Copied from uxtheme.h
//  If you have this new header, then delete these and
//  #include <uxtheme.h> instead!
//
#define ETDT_DISABLE        0x00000001
#define ETDT_ENABLE         0x00000002
#define ETDT_USETABTEXTURE  0x00000004
#define ETDT_ENABLETAB      (ETDT_ENABLE  | ETDT_USETABTEXTURE)

//
typedef HRESULT(WINAPI * ETDTProc) (HWND, DWORD);

//
//  Try to call EnableThemeDialogTexture, if uxtheme.dll is present
//
BOOL EnableDialogTheme(HWND hwnd)
{
    HMODULE hUXTheme;
    ETDTProc fnEnableThemeDialogTexture;

    hUXTheme = LoadLibrary(L"uxtheme.dll");

    if (hUXTheme)
    {
        fnEnableThemeDialogTexture =
            (ETDTProc)GetProcAddress(hUXTheme, "EnableThemeDialogTexture");

        if (fnEnableThemeDialogTexture)
        {
            fnEnableThemeDialogTexture(hwnd, ETDT_ENABLETAB);

            FreeLibrary(hUXTheme);
            return TRUE;
        }
        else
        {
            // Failed to locate API!
            FreeLibrary(hUXTheme);
            return FALSE;
        }
    }
    else
    {
        // Not running under XP? Just fail gracefully
        return FALSE;
    }
}

//
//  Get the specified file-version information string from a file
//
//  szItem  - version item string, e.g:
//      "FileDescription", "FileVersion", "InternalName",
//      "ProductName", "ProductVersion", etc  (see MSDN for others)
//
WCHAR *GetVersionString(WCHAR *szFileName, WCHAR *szValue, WCHAR *szBuffer, ULONG nLength)
{
    UINT  len;
    PVOID  ver;
    DWORD  *codepage;
    WCHAR  fmt[0x40];
    PVOID  ptr = 0;
    BOOL   result = FALSE;

    szBuffer[0] = '\0';

    len = GetFileVersionInfoSize(szFileName, 0);

    if (len == 0 || (ver = malloc(len)) == 0)
        return NULL;

    if (GetFileVersionInfo(szFileName, 0, len, ver))
    {
        if (VerQueryValue(ver, TEXT("\\VarFileInfo\\Translation"), (PVOID *)&codepage, &len))
        {
            swprintf_s(fmt, ARRAYSIZE(fmt), TEXT("\\StringFileInfo\\%04x%04x\\%s"), (*codepage) & 0xFFFF,
                (*codepage) >> 16, szValue);

            if (VerQueryValue(ver, fmt, &ptr, &len))
            {
                lstrcpyn(szBuffer, (WCHAR*)ptr, min(nLength, len));
                result = TRUE;
            }
        }
    }

    free(ver);
    return result ? szBuffer : NULL;
}


//
//  Compare Arch (32 or 64 bit) of our process with the process of the input window
//
BOOL ProcessArchMatches(HWND hwnd)
{
    static FARPROC fnIsWow64Process = NULL;
    static BOOL bIsWow64ProcessAbsents = FALSE;
    DWORD dwProcessId;
    HANDLE hProcess;
    BOOL bIsWow64Process;
    BOOL bSuccess;

    if (GetProcessorArchitecture() == PROCESSOR_ARCHITECTURE_INTEL)
        return TRUE;

    if (!fnIsWow64Process)
    {
        if (bIsWow64ProcessAbsents)
        {
#ifdef _WIN64
            return FALSE;
#else // ifndef _WIN64
            return TRUE;
#endif // _WIN64
        }

        fnIsWow64Process = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "IsWow64Process");
        if (!fnIsWow64Process)
        {
            bIsWow64ProcessAbsents = TRUE;

#ifdef _WIN64
            return FALSE;
#else // ifndef _WIN64
            return TRUE;
#endif // _WIN64
        }
    }

    GetWindowThreadProcessId(hwnd, &dwProcessId);

    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwProcessId);
    if (!hProcess)
        return FALSE; // assume no match, to be on the safe side

    bSuccess = ((BOOL(WINAPI *)(HANDLE, PBOOL))fnIsWow64Process)(hProcess, &bIsWow64Process);

    CloseHandle(hProcess);

    if (bSuccess)
    {
#ifdef _WIN64
        return !bIsWow64Process;
#else // ifndef _WIN64
        return bIsWow64Process;
#endif // _WIN64
    }
    else
        return FALSE; // assume no match, to be on the safe side
}


//
// Assumes to support only PROCESSOR_ARCHITECTURE_INTEL and PROCESSOR_ARCHITECTURE_AMD64
//
WORD GetProcessorArchitecture()
{
#ifdef _WIN64
    return PROCESSOR_ARCHITECTURE_AMD64;
#else // ifndef _WIN64
    static WORD wProcessorArchitecture = PROCESSOR_ARCHITECTURE_UNKNOWN;

    if (wProcessorArchitecture == PROCESSOR_ARCHITECTURE_UNKNOWN)
    {
        FARPROC fnGetNativeSystemInfo = NULL;
        SYSTEM_INFO siSystemInfo;

        fnGetNativeSystemInfo = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "GetNativeSystemInfo");

        if (fnGetNativeSystemInfo)
        {
            ((VOID(WINAPI *)(LPSYSTEM_INFO))fnGetNativeSystemInfo)(&siSystemInfo);

            wProcessorArchitecture = siSystemInfo.wProcessorArchitecture;
        }
        else
            wProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
    }

    return wProcessorArchitecture;
#endif // _WIN64
}


//
// Returns the real parent window
// Same as GetParent(), but doesn't return the owner
//
HWND GetRealParent(HWND hWnd)
{
    HWND hParent;

    hParent = GetAncestor(hWnd, GA_PARENT);
    if (!hParent || hParent == GetDesktopWindow())
        return NULL;

    return hParent;
}


//
// Copies text to clipboard
//
BOOL CopyTextToClipboard(HWND hWnd, WCHAR *psz)
{
    HGLOBAL hText;
    WCHAR *pszText;
    size_t nTextSize;

    nTextSize = wcslen(psz);

    hText = GlobalAlloc(GMEM_MOVEABLE, (nTextSize + 1) * sizeof(WCHAR));
    if (!hText)
        return FALSE;

    pszText = (WCHAR *)GlobalLock(hText);
    StringCchCopy(pszText, nTextSize + 1, psz);
    GlobalUnlock(hText);

    if (OpenClipboard(hWnd))
    {
        if (EmptyClipboard())
        {
            if (SetClipboardData(
                CF_UNICODETEXT,
                hText))
            {
                CloseClipboard();
                return TRUE;
            }
        }

        CloseClipboard();
    }

    GlobalFree(hText);
    return FALSE;
}

HBITMAP ExpandNineGridImage(SIZE outputSize, HBITMAP hbmSrc, RECT edges)
{
    HDC     hdcScreen, hdcDst, hdcSrc;
    HBITMAP hbmDst;
    void*   pBits;
    HANDLE  hOldSrc, hOldDst;
    BITMAP  bmSrc;

    // Create a 32bpp DIB of the desired size, this is the output bitmap.
    BITMAPINFOHEADER bih = { sizeof(bih) };

    bih.biWidth       = outputSize.cx;
    bih.biHeight      = outputSize.cy;
    bih.biPlanes      = 1;
    bih.biBitCount    = 32;
    bih.biCompression = BI_RGB;
    bih.biSizeImage   = 0;

    hdcScreen = GetDC(0);
    hbmDst = CreateDIBSection(hdcScreen, (BITMAPINFO *)&bih, DIB_RGB_COLORS, &pBits, 0, 0);

    // Determine size of the source image.
    GetObject(hbmSrc, sizeof(bmSrc), &bmSrc);

    // Prep DCs
    hdcSrc = CreateCompatibleDC(hdcScreen);
    hOldSrc = SelectObject(hdcSrc, hbmSrc);

    hdcDst = CreateCompatibleDC(hdcScreen);
    hOldDst = SelectObject(hdcDst, hbmDst);

    // Sizes of the nine-grid edges
    int cxEdgeL = edges.left;
    int cxEdgeR = edges.right;
    int cyEdgeT = edges.top;
    int cyEdgeB = edges.bottom;

    // Precompute sizes and coordinates of the interior boxes
    // (that is, the source and dest rects with the edges subtracted out).
    int cxDstInner = outputSize.cx - (cxEdgeL + cxEdgeR);
    int cyDstInner = outputSize.cy - (cyEdgeT + cyEdgeB);
    int cxSrcInner = bmSrc.bmWidth - (cxEdgeL + cxEdgeR);
    int cySrcInner = bmSrc.bmHeight - (cyEdgeT + cyEdgeB);

    int xDst1 = cxEdgeL;
    int xDst2 = outputSize.cx - cxEdgeR;
    int yDst1 = cyEdgeT;
    int yDst2 = outputSize.cy - cyEdgeB;

    int xSrc1 = cxEdgeL;
    int xSrc2 = bmSrc.bmWidth - cxEdgeR;
    int ySrc1 = cyEdgeT;
    int ySrc2 = bmSrc.bmHeight - cyEdgeB;

    // Upper-left corner
    BitBlt(
        hdcDst, 0, 0, cxEdgeL, cyEdgeT,
        hdcSrc, 0, 0,
        SRCCOPY);

    // Upper-right corner
    BitBlt(
        hdcDst, xDst2, 0, cxEdgeR, cyEdgeT,
        hdcSrc, xSrc2, 0,
        SRCCOPY);

    // Lower-left corner
    BitBlt(
        hdcDst, 0, yDst2, cxEdgeL, cyEdgeB,
        hdcSrc, 0, ySrc2,
        SRCCOPY);

    // Lower-right corner
    BitBlt(
        hdcDst, xDst2, yDst2, cxEdgeR, cyEdgeB,
        hdcSrc, xSrc2, ySrc2,
        SRCCOPY);

    // Left side
    StretchBlt(
        hdcDst, 0, yDst1, cxEdgeL, cyDstInner,
        hdcSrc, 0, ySrc1, cxEdgeL, cySrcInner,
        SRCCOPY);

    // Right side
    StretchBlt(
        hdcDst, xDst2, yDst1, cxEdgeR, cyDstInner,
        hdcSrc, xSrc2, ySrc1, cxEdgeR, cySrcInner,
        SRCCOPY);

    // Top side
    StretchBlt(
        hdcDst, xDst1, 0, cxDstInner, cyEdgeT,
        hdcSrc, xSrc1, 0, cxSrcInner, cyEdgeT,
        SRCCOPY);

    // Bottom side
    StretchBlt(
        hdcDst, xDst1, yDst2, cxDstInner, cyEdgeB,
        hdcSrc, xSrc1, ySrc2, cxSrcInner, cyEdgeB,
        SRCCOPY);

    // Middle
    StretchBlt(
        hdcDst, xDst1, yDst1, cxDstInner, cyDstInner,
        hdcSrc, xSrc1, ySrc1, cxSrcInner, cySrcInner,
        SRCCOPY);

    SelectObject(hdcSrc, hOldSrc);
    SelectObject(hdcDst, hOldDst);

    DeleteDC(hdcSrc);
    DeleteDC(hdcDst);

    ReleaseDC(0, hdcScreen);

    return hbmDst;
}

void UpdateLayeredWindowContent(HWND hwnd, RECT rc, HBITMAP hbmp, BYTE alpha)
{
    POINT ptZero = { 0, 0 };
    POINT pt;
    SIZE size;
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 0, AC_SRC_ALPHA };
    blend.SourceConstantAlpha = alpha;

    pt.x = rc.left;
    pt.y = rc.top;

    size.cx = GetRectWidth(&rc);
    size.cy = GetRectHeight(&rc);

    HDC hdcSrc = GetDC(0);
    HDC hdcMem = CreateCompatibleDC(hdcSrc);

    HANDLE hbmpOld = SelectObject(hdcMem, hbmp);

    UpdateLayeredWindow(
        hwnd,
        hdcSrc,
        &pt,
        &size,
        hdcMem,
        &ptZero,
        RGB(0, 0, 0),
        &blend,
        ULW_ALPHA);

    SelectObject(hdcMem, hbmpOld);
    DeleteDC(hdcMem);

    ReleaseDC(0, hdcSrc);
}


//
// Winforms wraps standard controls with a custom class name.
// Extract the underlying class name, e.g.:
//
//  WindowsForms10.SysTabControl32.app.0.fb11c8_r6_ad1
//     maps to:
//  SysTabControl32
//
// The buffer is modified in place.
//
// This is used to show the right window styles and pick nicer treeview
// icons for the cases where winforms is simply wrapping comctl32.
//

BOOL IsWindowsFormsClassName(PCWSTR pcszClass)
{
    return StrBeginsWith(pcszClass, L"WindowsForms");
}

void ExtractWindowsFormsInnerClassName(PWSTR pszName)
{
    if (StrBeginsWith(pszName, L"WindowsForms"))
    {
        WCHAR* pchStart = wcschr(pszName, '.');

        if (pchStart)
        {
            pchStart++;

            WCHAR *pchEnd = wcschr(pchStart, '.');

            // Found a substring that looks good, copy it to the front of the buffer.

            if (pchEnd)
            {
                *pchEnd = '\0';
                memmove(pszName, pchStart, (wcslen(pchStart) + 1) * sizeof(WCHAR));
            }
        }
    }
}
