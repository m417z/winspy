//
//  Edit Size Dialog.
//
//  Copyright (c) 2002 by J Brown
//  Freeware
//
//  Just a simple dialog which allows you to edit a
//  window's size / position
//

#include "WinSpy.h"

#include "resource.h"

//
//  rect - window coords
//
void SetupEdits(HWND hwndDlg, HWND hwndTarget, RECT *prect)
{
    DWORD dwStyle;
    RECT rect;

    CopyRect(&rect, prect);

    // Is this window a child control or not??
    dwStyle = GetWindowLong(hwndTarget, GWL_STYLE);

    // If this is a child window, then make its coords
    // relative to its parent.
    if (dwStyle & WS_CHILD)
        MapWindowPoints(NULL, GetParent(hwndTarget), (POINT *)&rect, 2);

    // Set the edit control's contents
    SetDlgItemInt(hwndDlg, IDC_EDITX, rect.left, TRUE);
    SetDlgItemInt(hwndDlg, IDC_EDITY, rect.top, TRUE);
    SetDlgItemInt(hwndDlg, IDC_EDITW, GetRectWidth(&rect), TRUE);
    SetDlgItemInt(hwndDlg, IDC_EDITH, GetRectHeight(&rect), TRUE);
}

//
//  Align the target window's size/pos and update the
//  contents of the edit boxes
//
void AlignTargetPos(HWND hwndDlg, HWND hwndTarget, UINT alignCommand)
{
    int x = GetDlgItemInt(hwndDlg, IDC_EDITX, 0, TRUE);
    int y = GetDlgItemInt(hwndDlg, IDC_EDITY, 0, TRUE);
    int w = GetDlgItemInt(hwndDlg, IDC_EDITW, 0, TRUE);
    int h = GetDlgItemInt(hwndDlg, IDC_EDITH, 0, TRUE);

    // Is this window a child control or not??
    DWORD dwStyle = GetWindowLong(hwndTarget, GWL_STYLE);

    // If this is a child window, then make its coords
    // relative to its parent.
    RECT containerRect;
    if (dwStyle & WS_CHILD)
    {
        HWND hwndParent = GetParent(hwndTarget);
        GetClientRect(hwndParent, &containerRect);
    }
    else
    {
        RECT rect = {x, y, x + w, y + h};
        HMONITOR monitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);

        MONITORINFO monitorInfo = { sizeof(monitorInfo) };
        GetMonitorInfo(monitor, &monitorInfo);

        CopyRect(&containerRect, &monitorInfo.rcWork);
    }

    switch (alignCommand) {
    case IDC_ADJUSTWINPOS_ALIGN_TOP:
        y = containerRect.top;
        break;

    case IDC_ADJUSTWINPOS_ALIGN_LEFT:
        x = containerRect.left;
        break;

    case IDC_ADJUSTWINPOS_ALIGN_CENTER:
        x = containerRect.left + (containerRect.right - containerRect.left - w) / 2;
        y = containerRect.top + (containerRect.bottom - containerRect.top - h) / 2;
        break;

    case IDC_ADJUSTWINPOS_ALIGN_RIGHT:
        x = containerRect.right - w;
        break;

    case IDC_ADJUSTWINPOS_ALIGN_BOTTOM:
        y = containerRect.bottom - h;
        break;
    }

    // Set the edit control's contents
    SetDlgItemInt(hwndDlg, IDC_EDITX, x, TRUE);
    SetDlgItemInt(hwndDlg, IDC_EDITY, y, TRUE);
    SetDlgItemInt(hwndDlg, IDC_EDITW, w, TRUE);
    SetDlgItemInt(hwndDlg, IDC_EDITH, h, TRUE);
}

//
//  Set the target window's size/pos, based on the
//  contents of the edit boxes
//
void SetTargetPos(HWND hwndDlg, HWND hwndTarget)
{
    int x = GetDlgItemInt(hwndDlg, IDC_EDITX, 0, TRUE);
    int y = GetDlgItemInt(hwndDlg, IDC_EDITY, 0, TRUE);
    int w = GetDlgItemInt(hwndDlg, IDC_EDITW, 0, TRUE);
    int h = GetDlgItemInt(hwndDlg, IDC_EDITH, 0, TRUE);

    SetWindowPos(hwndTarget, NULL, x, y, w, h, SWP_NOACTIVATE | SWP_NOZORDER);
}

//
//  Dialog procedure for the edit size window
//
INT_PTR CALLBACK EditSizeDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    RECT  rect;

    static HWND hwndTarget; // target window!
    static RECT rect0;      // original coords

    switch (iMsg)
    {
    case WM_INITDIALOG:

        hwndTarget = (HWND)lParam;

        GetWindowRect(hwndTarget, &rect0);
        CopyRect(&rect, &rect0);

        SetupEdits(hwnd, hwndTarget, &rect);

        // Set up the spin controls
        SendDlgItemMessage(hwnd, IDC_SPINX, UDM_SETRANGE, 0, MAKELONG(UD_MAXVAL, UD_MINVAL));
        SendDlgItemMessage(hwnd, IDC_SPINY, UDM_SETRANGE, 0, MAKELONG(UD_MAXVAL, UD_MINVAL));
        SendDlgItemMessage(hwnd, IDC_SPINW, UDM_SETRANGE, 0, MAKELONG(UD_MAXVAL, UD_MINVAL));
        SendDlgItemMessage(hwnd, IDC_SPINH, UDM_SETRANGE, 0, MAKELONG(UD_MAXVAL, UD_MINVAL));

        return TRUE;

    case WM_CLOSE:
        EndDialog(hwnd, 0);
        return TRUE;

    case WM_VSCROLL:

        SetTargetPos(hwnd, hwndTarget);

        // Get the window's coords again to see what happened
        GetWindowRect(hwndTarget, &rect);
        SetupEdits(hwnd, hwndTarget, &rect);

        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_RESET:

            // go back to the original coords
            SetupEdits(hwnd, hwndTarget, &rect0);
            SetTargetPos(hwnd, hwndTarget);

            // Get the window's coords again to see what happened
            GetWindowRect(hwndTarget, &rect);
            SetupEdits(hwnd, hwndTarget, &rect);

            return TRUE;

        case IDC_ADJUST:

            SetTargetPos(hwnd, hwndTarget);

            // Get the window's coords again to see what happened
            GetWindowRect(hwndTarget, &rect);
            SetupEdits(hwnd, hwndTarget, &rect);

            return TRUE;

        case IDC_ADJUSTWINPOS_ALIGN_TOP:
        case IDC_ADJUSTWINPOS_ALIGN_LEFT:
        case IDC_ADJUSTWINPOS_ALIGN_CENTER:
        case IDC_ADJUSTWINPOS_ALIGN_RIGHT:
        case IDC_ADJUSTWINPOS_ALIGN_BOTTOM:

            AlignTargetPos(hwnd, hwndTarget, LOWORD(wParam));
            SetTargetPos(hwnd, hwndTarget);

            // Get the window's coords again to see what happened
            GetWindowRect(hwndTarget, &rect);
            SetupEdits(hwnd, hwndTarget, &rect);

            return TRUE;

        case IDCANCEL:
            EndDialog(hwnd, 0);
            return TRUE;

        }

        return FALSE;
    }

    return FALSE;
}


void ShowEditSizeDlg(HWND hwndParent, HWND hwndTarget)
{
    if (IsWindow(g_hCurWnd))
    {
        DialogBoxParam(
            g_hInst,
            MAKEINTRESOURCE(IDD_ADJUSTWINPOS),
            hwndParent,
            EditSizeDlgProc,
            (LPARAM)hwndTarget);

        UpdateGeneralTab(hwndTarget);
    }
    else
    {
        MessageBox(hwndParent,
            L"Not a valid window",
            szAppName,
            MB_OK | MB_ICONEXCLAMATION);
    }
}
