//
//  WindowFromPointEx.c
//
//  Copyright (c) 2002 by J Brown
//  Freeware
//
//  Provides a better implementation of WindowFromPoint.
//  This function can return any window under the mouse,
//  including controls nested inside group-boxes, nested
//  dialogs etc.
//

#include "WinSpy.h"
#include "Utils.h"
#include "WindowFromPointEx.h"

typedef struct
{
    POINT pt;
    HWND  hwndBest;
    BOOL  fAllowHidden;
    DWORD dwArea;
} ChildSearchData;

//
//  Callback function used with FindBestChild
//
static BOOL CALLBACK FindBestChildProc(HWND hwnd, LPARAM lParam)
{
    ChildSearchData *pData = (ChildSearchData *)lParam;
    RECT rect;

    GetWindowRect(hwnd, &rect);

    // Is the mouse inside this child window?
    if (PtInRect(&rect, pData->pt))
    {
        // work out area of child window.
        // Width and height of any screen rectangle are guaranteed to be <32K each,
        // so their product is definitely much smaller than MAXINT
        DWORD a = GetRectWidth(&rect) * GetRectHeight(&rect);

        // if this child window is smaller than the
        // current "best", then choose this one
        if (a < pData->dwArea && (pData->fAllowHidden || IsWindowVisible(hwnd)))
        {
            pData->dwArea = a;
            pData->hwndBest = hwnd;
        }
    }

    return TRUE;
}

//
//  The problem:
//
//  WindowFromPoint API is not very good. It cannot cope
//  with odd window arrangements, i.e. a group-box in a dialog
//  may contain a few check-boxes. These check-boxes are not
//  children of the groupbox, but are at the same "level" in the
//  window hierarchy. WindowFromPoint will just return the
//  first available window it finds which encompasses the mouse
//  (i.e. the group-box), but will NOT be able to detect the contents.
//
//  Solution:
//
//  We use WindowFromPoint to start us off, and then step back one
//  level (i.e. from the parent of what WindowFromPoint returned).
//
//  Once we have this window, we enumerate ALL children of this window
//  ourselves, and find the one that best fits under the mouse -
//  the smallest window that fits, in fact.
//
//  I've tested this on a lot of different apps, and it seems
//  to work flawlessly - in fact, I haven't found a situation yet
//  that this method doesn't work on.....we'll see!
//
//  Inputs:
//
//  hwndFound - window found with WindowFromPoint
//  pt        - coordinates of mouse, in screen coords
//              (i.e. same coords used with WindowFromPoint)
//  fAllowHidden - whether to include hidden windows in the search
//
static HWND FindBestChild(HWND hwndFound, POINT pt, BOOL fAllowHidden)
{
    HWND  hwnd;
    DWORD dwStyle;

    ChildSearchData data;
    data.fAllowHidden = fAllowHidden;
    data.dwArea = MAXUINT;
    data.hwndBest = 0;
    data.pt = pt;

    hwnd = GetParent(hwndFound);

    dwStyle = GetWindowLong(hwndFound, GWL_STYLE);

    // The original window might already be a top-level window,
    // so we don't want to start at *its* parent
    if (hwnd == 0 || (dwStyle & WS_POPUP))
        hwnd = hwndFound;

    // Enumerate EVERY child window.
    //
    //  Note to reader:
    //
    //  You can get some real interesting effects if you set
    //  hwnd = GetDesktopWindow()
    //  fAllowHidden = TRUE
    //  ...experiment!!
    //
    EnumChildWindows(hwnd, FindBestChildProc, (LPARAM)&data);

    if (data.hwndBest == 0)
        data.hwndBest = hwnd;

    return data.hwndBest;
}

//
//  Find window under specified point (screen coordinates)
//
HWND WindowFromPointEx(POINT pt, BOOL fTopLevel, BOOL fAllowHidden)
{
    HWND hWndPoint;

    //
    // First of all find the parent window under the mouse
    // We are working in SCREEN coordinates
    //
    hWndPoint = WindowFromPoint(pt);

    if (hWndPoint == 0)
        return 0;

    if (fTopLevel)
    {
        hWndPoint = GetAncestor(hWndPoint, GA_ROOT);
    }
    else
    {
        // WindowFromPoint is not too accurate. There is quite likely
        // another window under the mouse.
        hWndPoint = FindBestChild(hWndPoint, pt, fAllowHidden);

        //if we don't allow hidden windows, then return the parent
        if (!fAllowHidden)
        {
            while (hWndPoint && !IsWindowVisible(hWndPoint))
                hWndPoint = GetRealParent(hWndPoint);
        }
    }

    return hWndPoint;
}
