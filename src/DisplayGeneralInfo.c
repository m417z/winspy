//
//  DisplayGeneralInfo.c
//  Copyright (c) 2002 by J Brown
//  Freeware
//
//  Fill the general-tab-pane with general info for the
//  specified window
//

#include "WinSpy.h"

#include "resource.h"
#include "Utils.h"

void RemoveHyperlink(HWND hwnd, UINT staticid);
void MakeHyperlink(HWND hwnd, UINT staticid, COLORREF crLink);
void FillBytesList(
    HWND hwndDlg,
    HWND hwnd,
    int numBytes,
    WORD WINAPI pGetWord(HWND, int),
    LONG WINAPI pGetLong(HWND, int),
    LONG_PTR WINAPI pGetLongPtr(HWND, int)
);

//
// Three possible states:
//
// 1. The wndproc isn't known and we have not yet tried to get it.
//    The "N/A" link control is shown.
//
// 2. We tried to fetch the wndproc via thread injection and failed.
//    The non-link control is shown with a value of "N/A".
//
// 3. We know the wndproc.
//    Show the non-link control with the value of the wndproc.
//

void UpdateWndProcControls(HWND hwnd, HWND hwndDlg, PVOID clsproc)
{
    WCHAR ach[100];

    // If we don't know the wndproc and have not already attempted the
    // remote thread injection, then show the link.

    BOOL fShowLink = (!g_WndProc && !g_fTriedRemote);

    ShowDlgItem(hwndDlg, IDC_WNDPROC_LINK, fShowLink ? SW_SHOW : SW_HIDE);
    ShowDlgItem(hwndDlg, IDC_WNDPROC,      fShowLink ? SW_HIDE : SW_SHOW);

    if (g_WndProc == 0)
    {
        swprintf_s(ach, ARRAYSIZE(ach), L"N/A");
    }
    else
    {
        swprintf_s(ach, ARRAYSIZE(ach), L"%p", g_WndProc);

        if (clsproc == NULL)
        {
            clsproc = (PVOID)(IsWindowUnicode(hwnd) ? GetClassLongPtrW : GetClassLongPtrA)(hwnd, GCLP_WNDPROC);
        }

        if (clsproc && (g_WndProc != clsproc))
        {
            wcscat_s(ach, ARRAYSIZE(ach), L" (Subclassed)");
        }
    }

    SetDlgItemTextEx(hwndDlg, IDC_WNDPROC_LINK, ach);
    SetDlgItemTextEx(hwndDlg, IDC_WNDPROC, ach);
}


//
// Clears all the controls on the tab (except the handle value) because either
// there is no current window, or the current window is invalid.
//

void ResetGeneralTab(HWND hwnd, HWND hwndDlg)
{
    // Reset the labels to blank or '(invalid window)'

    PCWSTR pszMessage = hwnd ? szInvalidWindow : L"";

    SetDlgItemTextEx(hwndDlg, IDC_CAPTION1,       pszMessage);
    SetDlgItemTextEx(hwndDlg, IDC_CAPTION2,       pszMessage);
    SetDlgItemTextEx(hwndDlg, IDC_CLASS,          pszMessage);
    SetDlgItemTextEx(hwndDlg, IDC_STYLE,          pszMessage);
    SetDlgItemTextEx(hwndDlg, IDC_RECTANGLE,      pszMessage);
    SetDlgItemTextEx(hwndDlg, IDC_CLIENTRECT,     pszMessage);
    SetDlgItemTextEx(hwndDlg, IDC_WNDPROC_LINK,   pszMessage);
    SetDlgItemTextEx(hwndDlg, IDC_WNDPROC,        pszMessage);
    SetDlgItemTextEx(hwndDlg, IDC_INSTANCE,       pszMessage);
    SetDlgItemTextEx(hwndDlg, IDC_USERDATA,       pszMessage);
    SetDlgItemTextEx(hwndDlg, IDC_CONTROLID,      pszMessage);

    // Reset controls to default states.

    ShowDlgItem(hwndDlg, IDC_CAPTION1, SW_SHOW);
    ShowDlgItem(hwndDlg, IDC_CAPTION2, SW_HIDE);
    SendDlgItemMessage(hwndDlg, IDC_CAPTION2, CB_RESETCONTENT, 0, 0);

    ShowDlgItem(hwndDlg, IDC_WNDPROC_LINK, SW_HIDE);
    ShowDlgItem(hwndDlg, IDC_WNDPROC, SW_SHOW);

    SendDlgItemMessage(hwndDlg, IDC_BYTESLIST, CB_RESETCONTENT, 0, 0);
    EnableDlgItem(hwndDlg, IDC_BYTESLIST, FALSE);
}


void UpdateGeneralTab(HWND hwnd)
{
    WCHAR   ach[256];
    HWND    hwndDlg = WinSpyTab[GENERAL_TAB].hwnd;
    RECT    rect;

    *ach = 0;
    ZeroMemory(&rect, sizeof(rect));

    // Handle

    if (hwnd)
    {
        swprintf_s(ach, ARRAYSIZE(ach), L"%08X", (UINT)(UINT_PTR)hwnd);
    }

    SetDlgItemTextEx(hwndDlg, IDC_HANDLE, ach);

    // This must come after filling in the handle.

    if (!hwnd || !IsWindow(hwnd))
    {
        ResetGeneralTab(hwnd, hwndDlg);
        return;
    }

    // Caption
    ShowDlgItem(hwndDlg, IDC_CAPTION1, SW_SHOW);
    ShowDlgItem(hwndDlg, IDC_CAPTION2, SW_HIDE);

    SendDlgItemMessage(hwndDlg, IDC_CAPTION2, CB_RESETCONTENT, 0, 0);

    // SendMessage is better than GetWindowText,
    // because it gets text of children in other processes
    if (g_fPassword)
    {
        // For password edit controls, we try thread injection.

        GetRemoteInfo();
        wcscpy_s(ach, ARRAYSIZE(ach), g_szPassword);
    }
    else
    {
        ach[0] = 0;

        if (!SendMessageTimeout(hwnd, WM_GETTEXT, ARRAYSIZE(ach), (LPARAM)ach,
            SMTO_ABORTIFHUNG, 100, NULL))
        {
            GetWindowText(hwnd, ach, ARRAYSIZE(ach));
        }

        // WM_GETTEXT does not guarantee null termination.

        ach[ARRAYSIZE(ach) - 1] = '\0';
    }

    SetDlgItemTextEx(hwndDlg, IDC_CAPTION1, ach); // edit box
    SetDlgItemTextEx(hwndDlg, IDC_CAPTION2, ach); // combo box

    // Class name

    GetClassName(hwnd, ach, ARRAYSIZE(ach));
    VerboseClassName(ach, ARRAYSIZE(ach), (WORD)GetClassLong(hwnd, GCW_ATOM));

    if (IsWindowUnicode(hwnd))
    {
        wcscat_s(ach, ARRAYSIZE(ach), L"  (Unicode)");
    }

    SetDlgItemTextEx(hwndDlg, IDC_CLASS, ach);

    // Style

    swprintf_s(ach, ARRAYSIZE(ach), L"%08X", GetWindowLong(hwnd, GWL_STYLE));
    wcscat_s(ach, ARRAYSIZE(ach), IsWindowVisible(hwnd) ? L"  (visible, " : L"  (hidden, ");
    wcscat_s(ach, ARRAYSIZE(ach), IsWindowEnabled(hwnd) ? L"enabled" : L"disabled");

    DWORD dwCloaked = 0;
    DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &dwCloaked, sizeof(dwCloaked));

    if (dwCloaked & DWM_CLOAKED_APP)
    {
        wcscat_s(ach, ARRAYSIZE(ach), L", app cloaked");
    }
    else if (dwCloaked & DWM_CLOAKED_SHELL)
    {
        wcscat_s(ach, ARRAYSIZE(ach), L", cloaked");
    }

    wcscat_s(ach, ARRAYSIZE(ach), L")");

    SetDlgItemTextEx(hwndDlg, IDC_STYLE, ach);

    // Window rect

    GetWindowRect(hwnd, &rect);
    int x1 = rect.left;
    int y1 = rect.top;

    FormatDlgItemText(
        hwndDlg, IDC_RECTANGLE,
        L"(%d,%d) - (%d,%d)  -  %dx%d",
        rect.left, rect.top, rect.right, rect.bottom,
        GetRectWidth(&rect), GetRectHeight(&rect));

    // Client rect

    RECT rcClient;

    GetClientRect(hwnd, &rcClient);
    MapWindowPoints(hwnd, 0, (POINT *)&rcClient, 2);

    if (!g_fShowClientRectAsMargins)
    {
        x1 = rcClient.left - x1;
        y1 = rcClient.top - y1;

        OffsetRect(&rcClient, -rcClient.left, -rcClient.top);
        OffsetRect(&rcClient, x1, y1);

        swprintf_s(ach, ARRAYSIZE(ach), L"(%d,%d) - (%d,%d)  -  %dx%d",
            rcClient.left, rcClient.top, rcClient.right, rcClient.bottom,
            GetRectWidth(&rcClient), GetRectHeight(&rcClient));
    }
    else
    {
        swprintf_s(ach, ARRAYSIZE(ach), L"{ %d, %d, %d, %d }",
            rcClient.left - rect.left,
            rcClient.top - rect.top,
            rect.right - rcClient.right,
            rect.bottom - rcClient.bottom);
    }

    SetDlgItemTextEx(hwndDlg, IDC_CLIENTRECT, ach);

    //restored rect
    /*GetWindowPlacement(hwnd, &wp);
    wsprintf(ach, L"(%d,%d) - (%d,%d)  -  %dx%d",
        wp.rcNormalPosition.left, wp.rcNormalPosition.top,
        wp.rcNormalPosition.right, wp.rcNormalPosition.bottom,
        (wp.rcNormalPosition.right-wp.rcNormalPosition.left),
        (wp.rcNormalPosition.bottom-wp.rcNormalPosition.top));

    SetDlgItemText(hwndDlg, IDC_RESTOREDRECT, ach);*/

    // Window procedure

    UpdateWndProcControls(hwnd, hwndDlg, NULL);

    // Instance handle

    LONG_PTR lp = GetWindowLongPtr(hwnd, GWLP_HINSTANCE);

    FormatDlgItemText(hwndDlg, IDC_INSTANCE, L"%p", (void*)lp);

    // User data

    lp = GetWindowLongPtr(hwnd, GWLP_USERDATA);

    FormatDlgItemText(hwndDlg, IDC_USERDATA, L"%p", (void*)lp);

    // Control ID
    //
    // despite the name "GWLP_ID" suggesting that control ID is pointer-sized,
    // it would only work properly in WM_COMMAND if it was a WORD,
    // as it is passed in LOWORD(wParam)

    lp = GetWindowLongPtr(hwnd, GWLP_ID);

    FormatDlgItemText(hwndDlg, IDC_CONTROLID, L"%04IX  (%Id)", lp, lp);

    // Extra window bytes

    int numbytes = GetClassLong(hwnd, GCL_CBWNDEXTRA);

    FillBytesList(hwndDlg, hwnd, numbytes, GetWindowWord, GetWindowLong, GetWindowLongPtr);
}
