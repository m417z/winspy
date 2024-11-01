//
//  WinSpy.c
//
//  Copyright (c) 2002 by J Brown
//  Freeware
//
//  Main implementation.
//
//  v 1.7.1 - moved system-menu items to appear before the Close item
//

#include "WinSpy.h"

#include "resource.h"
#include "FindTool.h"
#include "CaptureWindow.h"
#include "BitmapButton.h"
#include "Utils.h"
#include "WindowFromPointEx.h"
#include "Poster.h"


HWND       g_hwndMain;       // Main winspy window
HWND       g_hwndPin;        // Toolbar with pin bitmap
HWND       g_hwndSizer;      // Sizing grip for bottom-right corner
HWND       g_hwndToolTip;    // tooltip for main window controls only
HINSTANCE  g_hInst;          // Current application instance
BOOL       g_fFirstInstance;

//
//  Current window being spied on
//
HWND       g_hCurWnd = 0;
WNDPROC    g_WndProc;
BOOL       g_fPassword = FALSE;     // is it a password (edit) control?
BOOL       g_fTriedRemote = FALSE;
WCHAR      g_szPassword[200];
WCHAR      g_szClassName[MAX_PATH];
DWORD      g_dwSelectedPID;         // Set only when a process node is selected in the treeview
BOOL       g_fShowClientRectAsMargins = FALSE;

static TBBUTTON tbbPin[] =
{
    {   0,  IDM_WINSPY_PIN,     TBSTATE_ENABLED, TBSTYLE_CHECK,  0, 0   },
};

#define IDC_PIN_TOOLBAR 2000 // must be unique, so check resource.h
#define TOOLBAR_PIN_STYLES  (TBSTYLE_FLAT | WS_CHILD | WS_VISIBLE | \
                        CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_NODIVIDER)

DialogTab WinSpyTab[NUMTABCONTROLITEMS] =
{
    0, L"General",       IDD_TAB_GENERAL,    GeneralDlgProc,
    0, L"Styles",        IDD_TAB_STYLES,     StyleDlgProc,
    0, L"Properties",    IDD_TAB_PROPERTIES, PropertyDlgProc,
    0, L"Class",         IDD_TAB_CLASS,      ClassDlgProc,
    0, L"Windows",       IDD_TAB_WINDOWS,    WindowDlgProc,
    0, L"Process",       IDD_TAB_PROCESS,    ProcessDlgProc,
    0, L"DPI",           IDD_TAB_DPI,        DpiDlgProc,
};

static int nCurrentTab = 0;

//
// This function tries to get additional data about the current window by
// injecting a thread into the process that owns the window.
// Currently, this is used for two things:
//
// 1. The window procedure.  There seems to be no way to query the window
//    procedure for a window from outside the process that owns the window.
//
// 2. Edit controls with the ES_PASSWORD style.
//
// Note that this can fail for various reasons:
// - The other process if of a different bitness than winspy (32 vs. 64)
// - The other process could be suspend or sitting in a debugger.
// - We may not have sufficient rights to open a handle to the other process.
//

void GetRemoteInfo()
{
    HWND hwnd = g_hCurWnd;

    if (!hwnd || g_fTriedRemote)
        return;

    if (g_WndProc == 0 || g_fPassword)
    {
        BOOL fAttemptInjection = ProcessArchMatches(hwnd);

        // Skip console windows.  For these, the system tells us that they are
        // owned by the associated console process (e.g cmd.exe) and that
        // process won't necessarily have user32 loaded.  GetRemoteWindowInfo
        // will crash the target process in that case, so don't try.

        if (wcscmp(g_szClassName, L"ConsoleWindowClass") == 0)
        {
            fAttemptInjection = FALSE;
        }

        if (fAttemptInjection)
        {
            GetRemoteWindowInfo(hwnd, NULL, g_WndProc ? NULL : &g_WndProc, g_szPassword, ARRAYSIZE(g_szPassword));
        }
        else if (g_fPassword)
        {
            SendMessage(hwnd, WM_GETTEXT, ARRAYSIZE(g_szPassword), (LPARAM)g_szPassword);

            // WM_GETTEXT does not guarantee null termination.

            g_szPassword[ARRAYSIZE(g_szPassword) - 1] = '\0';
        }
    }

    g_fTriedRemote = TRUE;
}

void UpdateActiveTab()
{
    HWND hwnd = g_hCurWnd;

    if (nCurrentTab == GENERAL_TAB)
    {
        UpdateGeneralTab(hwnd);
    }
    else if (nCurrentTab == CLASS_TAB)
    {
        UpdateClassTab(hwnd);
    }
    else if (nCurrentTab == STYLE_TAB)
    {
        UpdateStyleTab(hwnd);
    }
    else if (nCurrentTab == PROPERTY_TAB)
    {
        UpdatePropertyTab(hwnd);
    }
    else if (nCurrentTab == PROCESS_TAB)
    {
        UpdateProcessTab(hwnd, g_dwSelectedPID);
    }
    else if (nCurrentTab == WINDOW_TAB)
    {
        UpdateWindowTab(hwnd);
    }
    else if (nCurrentTab == DPI_TAB)
    {
        UpdateDpiTab(hwnd);
    }

    WindowTree_RefreshWindowNode(hwnd);
}

//
//  Top-level function for retrieving+displaying a window's
//  information (styles/class/properties etc)
//
void DisplayWindowInfo(HWND hwnd)
{
    // These globals are updated only when the selected window changes.

    if (hwnd != g_hCurWnd)
    {
        g_hCurWnd        = hwnd;
        g_szClassName[0] = '\0';
        g_WndProc        = NULL;
        g_fPassword      = FALSE;
        g_fTriedRemote   = FALSE;

        if (hwnd)
        {
            if (!GetClassName(hwnd, g_szClassName, ARRAYSIZE(g_szClassName)))
            {
                g_szClassName[0] = '\0';
            }

            // Attempt to query the window proc.  Note that this only works
            // in-proc, so this is really just handling the edge case where
            // you are looking at a window within winspy itself.  In all
            // other cases the wndproc isn't available without the thread
            // injection.

            g_WndProc = (WNDPROC)(IsWindowUnicode(hwnd) ? GetWindowLongPtrW : GetWindowLongPtrA)(hwnd, GWLP_WNDPROC);

            // If a password-edit control, then we must inject a thread into
            // the other process to query the window text.

            if (lstrcmpi(g_szClassName, L"Edit") == 0)
            {
                DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);

                if (dwStyle & ES_PASSWORD)
                    g_fPassword = TRUE;
            }
        }
    }

    UpdateMainWindowText();

    UpdateActiveTab();
}

void UpdateMainWindowText()
{
    HWND hwnd = g_hCurWnd;

    if (hwnd && g_opts.fShowInCaption)
    {
        WCHAR szClass[70] = { 0 };
        WCHAR ach[100] = { 0 };

        if (IsWindow(hwnd))
        {
            GetClassName(hwnd, szClass, ARRAYSIZE(szClass));
        }
        else
        {
             wcscpy_s(szClass, ARRAYSIZE(szClass), szInvalidWindow);
        }

        swprintf_s(ach, ARRAYSIZE(ach), L"%s [%08x, %s]", szAppName, (UINT)(UINT_PTR)hwnd, szClass);
        SetWindowText(g_hwndMain, ach);
    }
    else
    {
        SetWindowText(g_hwndMain, szAppName);
    }
}

//
//  User-defined callback function for the Find Tool
//
UINT CALLBACK WndFindProc(HWND hwndTool, UINT uCode, HWND hwnd)
{
    HWND hwndMain = GetParent(hwndTool);

    static BOOL fFirstDrag = TRUE;
    static HWND hwndLastTarget;
    static BOOL fOldShowHidden;
    static BOOL fWasMinimized;
    static UINT uLastLayout;
    BOOL fCanceled = FALSE;

    switch (uCode)
    {
    case WFN_SELCHANGED:

        DisplayWindowInfo(hwnd);
        return 0;

    case WFN_BEGIN:

        hwndLastTarget = g_hCurWnd;

        fWasMinimized = g_opts.fMinimizeWinSpy;
        if (fWasMinimized)
        {
            uLastLayout = GetWindowLayout(hwndMain);
            SetWindowLayout(hwndMain, WINSPY_MINIMIZED);
            InvalidateRect(hwndMain, 0, 0);
            UpdateWindow(hwndMain);
        }

        fOldShowHidden = g_opts.fShowHidden;
        break;

    case WFN_CANCELLED:

        fCanceled = TRUE;
        // Restore the current window + Fall through!
        g_hCurWnd = hwndLastTarget;

    case WFN_END:

        ShowWindow(hwndMain, SW_SHOW);

        if (fWasMinimized)
            SetWindowLayout(hwndMain, uLastLayout);

        if (!fCanceled && fFirstDrag)
        {
            fFirstDrag = FALSE;
            SetWindowLayout(hwndMain, WINSPY_LASTMAX);
        }

        DisplayWindowInfo(g_hCurWnd);

        g_opts.fShowHidden = fOldShowHidden;
        break;

    case WFN_CTRL_UP:
        ToggleWindowLayout(hwndMain);
        break;

    case WFN_CTRL_DOWN:
        ToggleWindowLayout(hwndMain);
        break;

    case WFN_SHIFT_UP:
        SetWindowPos(hwndMain, 0, 0, 0, 0, 0, SWP_SHOWONLY);
        g_opts.fShowHidden = fOldShowHidden;
        break;

    case WFN_SHIFT_DOWN:
        SetWindowPos(hwndMain, 0, 0, 0, 0, 0, SWP_HIDEONLY);
        g_opts.fShowHidden = TRUE;
        break;

    }
    return 0;
}

//
// Check/Uncheck the specified item in the system menu
//
void CheckSysMenu(HWND hwnd, UINT uItemId, BOOL fChecked)
{
    HMENU hSysMenu;

    hSysMenu = GetSystemMenu(hwnd, FALSE);

    if (fChecked)
        CheckMenuItem(hSysMenu, uItemId, MF_CHECKED | MF_BYCOMMAND);
    else
        CheckMenuItem(hSysMenu, uItemId, MF_UNCHECKED | MF_BYCOMMAND);
}

//
// Is the specified item in system menu checked?
//
BOOL IsSysMenuChecked(HWND hwnd, UINT uItemId)
{
    HMENU hSysMenu;
    DWORD dwState;

    hSysMenu = GetSystemMenu(hwnd, FALSE);

    dwState = GetMenuState(hSysMenu, uItemId, MF_BYCOMMAND);

    return (dwState & MF_CHECKED) ? TRUE : FALSE;
}

//
//  Toggle the checked status for specified item
//
BOOL ToggleSysMenuCheck(HWND hwnd, UINT uItemId)
{
    if (IsSysMenuChecked(hwnd, uItemId))
    {
        CheckSysMenu(hwnd, uItemId, FALSE);
        return FALSE;
    }
    else
    {
        CheckSysMenu(hwnd, uItemId, TRUE);
        return TRUE;
    }
}

//
//  Determine the window layout and check/uncheck  the
//  maximized menu item accordingly
//
void SetSysMenuIconFromLayout(HWND hwnd, UINT layout)
{
    if (layout == WINSPY_MINIMIZED)
        CheckSysMenu(hwnd, SC_MAXIMIZE, FALSE);
    else
        CheckSysMenu(hwnd, SC_MAXIMIZE, TRUE);
}

//
//  Create a sizing grip for the lower-right corner
//
HWND CreateSizeGrip(HWND hwndDlg)
{
    HWND hwndSizeGrip;

    // Create a sizing grip for the lower-right corner
    hwndSizeGrip = CreateWindow(
        L"Scrollbar",
        L"",
        WS_VISIBLE | WS_CHILD | SBS_SIZEGRIP |
        SBS_SIZEBOXBOTTOMRIGHTALIGN | WS_CLIPSIBLINGS,
        0, 0, 20, 20,
        hwndDlg, 0, g_hInst, 0);

    return hwndSizeGrip;
}

//
//  Create a tooltip control,
//
HWND CreateTooltip(HWND hwndDlg)
{
    HWND hwndTT;
    TOOLINFO ti;
    int i;
    BOOL fRet;

    struct CtrlTipsTag
    {
        int   uDlgId;   // -1 for main window, 0 through n for tab dialogs
        UINT  uCtrlId;
        PWSTR szText;

    } CtrlTips[] =
    {
        -1, IDC_DRAGGER,    L"Window Finder Tool",
        -1, IDC_PIN_TOOLBAR,L"Keep On-Screen (F4)",
        -1, IDC_MINIMIZE,   L"Minimize On Use",
        -1, IDC_HIDDEN,     L"Display Hidden Windows",
        -1, IDC_CAPTURE,    L"Capture Current Window (Alt+C)",
        -1, IDC_AUTOUPDATE, L"Update data every second",
        -1, IDC_EXPAND,     L"Expand / Collapse (F3)",
        -1, IDC_REFRESH,    L"Refresh Window List (F6)",
        -1, IDC_LOCATE,     L"Locate Current Window",
        -1, IDC_FLASH,      L"Highlight Selected Window",

        GENERAL_TAB, IDC_HANDLE_MENU,  L"Window Commands",
        GENERAL_TAB, IDC_SETCAPTION,   L"Set Window Caption",
        GENERAL_TAB, IDC_EDITSIZE,     L"Adjust Window Position",
        GENERAL_TAB, IDC_WNDPROC_LINK, L"Not Available.  Click to try injecting code into owning process.",
        CLASS_TAB,   IDC_WNDPROC_LINK, L"Not Available.  Click to try injecting code into owning process.",
        STYLE_TAB,   IDC_EDITSTYLE,    L"Edit Styles",
        STYLE_TAB,   IDC_EDITSTYLEEX,  L"Edit Extended Styles",
        PROCESS_TAB, IDC_PROCESS_MENU, L"Process Commands",
        WINDOW_TAB,  IDC_PARENT,       L"Parent Window",
        WINDOW_TAB,  IDC_OWNER,        L"Owner Window",
    };

    // Create tooltip for main window
    hwndTT = CreateWindowEx(WS_EX_TOPMOST,
        TOOLTIPS_CLASS,
        NULL,
        WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        hwndDlg,
        NULL,
        g_hInst,
        NULL
    );

    //
    //  Add tooltips to every control (above)
    //
    for (i = 0; i < ARRAYSIZE(CtrlTips); i++)
    {
        HWND hwnd;

        if (CtrlTips[i].uDlgId == -1)
            hwnd = hwndDlg;
        else
            hwnd = WinSpyTab[CtrlTips[i].uDlgId].hwnd;

        ti.cbSize = sizeof(ti);
        ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
        ti.hwnd = hwnd;
        ti.uId = (UINT_PTR)GetDlgItem(hwnd, CtrlTips[i].uCtrlId);
        ti.hinst = g_hInst;
        ti.lpszText = CtrlTips[i].szText;
        ti.lParam = 0;

        fRet = (BOOL)SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)&ti);
    }

    SendMessage(hwndTT, TTM_ACTIVATE, g_opts.fEnableToolTips, 0);

    return hwndTT;
}

//
//  Create a toolbar with one button in it, for
//  the pin-button
//
HWND CreatePinToolbar(HWND hwndDlg)
{
    RECT    rect;
    HWND    hwndTB;

    // Create the toolbar to hold pin bitmap
    hwndTB = CreateToolbarEx(
        hwndDlg,
        TOOLBAR_PIN_STYLES,             //,
        IDC_PIN_TOOLBAR,                //toolbar ID (don't need)
        2,                              //number of button images
        g_hInst,                          //where the bitmap is
        IDB_PIN_BITMAP,                 //bitmap resource name
        tbbPin,                         //TBBUTTON structure
        ARRAYSIZE(tbbPin),
        15, 14, 15, 14,                 //0,0,//16,18, 16, 18,
        sizeof(TBBUTTON));


    // Find out how big the button is, so we can resize the
    // toolbar to fit perfectly
    SendMessage(hwndTB, TB_GETITEMRECT, 0, (LPARAM)&rect);

    SetWindowPos(hwndTB, HWND_TOP, 0, 0,
        GetRectWidth(&rect),
        GetRectHeight(&rect), SWP_NOMOVE);

    // Setup the bitmap image
    SendMessage(hwndTB, TB_CHANGEBITMAP, IDM_WINSPY_PIN,
        (LPARAM)MAKELPARAM(g_opts.fPinWindow, 0));

    // Checked / Unchecked
    SendMessage(hwndTB, TB_CHECKBUTTON, IDM_WINSPY_PIN, MAKELONG(g_opts.fPinWindow, 0));

    return hwndTB;
}

//
//  WM_INITDIALOG handler
//
BOOL WinSpy_InitDlg(HWND hwnd)
{
    HBITMAP hBmp1, hBmp2;
    HMENU   hSysMenu;
    int     i;
    HICON   hIcon;
    TCITEM  tcitem;

    g_hwndMain = hwnd;

    // Initialize the finder tool
    MakeFinderTool(GetDlgItem(hwnd, IDC_DRAGGER), WndFindProc);

    // Make the More>> button into a bitmap
    MakeDlgBitmapButton(hwnd, IDC_EXPAND, IDI_MORE);

    g_hwndSizer = CreateSizeGrip(hwnd);
    g_hwndPin = CreatePinToolbar(hwnd);

    // Load image lists etc
    WindowTree_Initialize(GetDlgItem(hwnd, IDC_TREE1));

    // Create each dialog-tab pane,
    for (i = 0; i < NUMTABCONTROLITEMS; i++)
    {
        ZeroMemory(&tcitem, sizeof(tcitem));

        tcitem.mask = TCIF_TEXT;
        tcitem.pszText = (PWSTR)WinSpyTab[i].szText;

        // Create the dialog pane
        WinSpyTab[i].hwnd = CreateDialog(g_hInst,
            MAKEINTRESOURCE(WinSpyTab[i].id), hwnd, WinSpyTab[i].dlgproc);

        // Create the corresponding tab
        SendDlgItemMessage(hwnd, IDC_TAB1, TCM_INSERTITEM, i, (LPARAM)&tcitem);

        SetWindowText(WinSpyTab[i].hwnd, WinSpyTab[i].szText);

        // Make this dialog XP-theme aware!
        EnableDialogTheme(WinSpyTab[i].hwnd);
    }

    InitStockStyleLists();

    //
    CheckDlgButton(hwnd, IDC_MINIMIZE, g_opts.fMinimizeWinSpy);
    CheckDlgButton(hwnd, IDC_HIDDEN, g_opts.fShowHidden);

    // Position our contents, work out how big the various
    // layouts are (depending on current system settings for border
    // width, titlebar height etc).
    WinSpyDlg_SizeContents(hwnd);

    // Make sure we test for this FIRST, before we do ANY SetWindowPos'.
    // Otherwise, we will get a WM_WINDOWPOSCHANGED, and set fAlwaysOnTop to 0!
    if (g_opts.fAlwaysOnTop)
    {
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_ZONLY);
    }

    // See what the registry settings are, and setup accordingly
    if (g_opts.fSaveWinPos && g_opts.ptPinPos.x != CW_USEDEFAULT && g_opts.ptPinPos.y != CW_USEDEFAULT)
    {
        SetWindowLayout(hwnd, WINSPY_MINIMIZED);
    }
    else
    {
        RECT rect;
        GetWindowRect(hwnd, &rect);
        g_opts.ptPinPos.x = rect.left;
        g_opts.ptPinPos.y = rect.top;
    }

    // display the general tab
    SetWindowPos(WinSpyTab[0].hwnd, 0, 0, 0, 0, 0, SWP_SHOWONLY);

    //
    //  Customize this window's system menu by adding our own
    //  commands.
    //
    hSysMenu = GetSystemMenu(hwnd, FALSE);


    // add items *before* the close item
    InsertMenu(hSysMenu, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED | MF_STRING, IDM_WINSPY_BROADCASTER, L"&Broadcaster");
    InsertMenu(hSysMenu, SC_CLOSE, MF_BYCOMMAND | MF_SEPARATOR, (UINT_PTR)-1, L"");
    InsertMenu(hSysMenu, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED | MF_STRING, IDM_WINSPY_ABOUT, L"&About");
    InsertMenu(hSysMenu, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED | MF_STRING, IDM_WINSPY_OPTIONS, L"&Options...\tAlt+Enter");
    InsertMenu(hSysMenu, SC_CLOSE, MF_BYCOMMAND | MF_SEPARATOR, (UINT_PTR)-1, L"");
    InsertMenu(hSysMenu, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED | MF_STRING |
        (g_opts.fAlwaysOnTop ? MF_CHECKED : 0), IDM_WINSPY_ONTOP,
        L"Always On &Top\tShift+Y");
    InsertMenu(hSysMenu, SC_CLOSE, MF_BYCOMMAND | MF_SEPARATOR, (UINT_PTR)-1, L"");

    // Change the Maximize item to a Toggle Layout item
    ModifyMenu(hSysMenu, SC_MAXIMIZE, MF_ENABLED | MF_STRING, SC_MAXIMIZE,
        L"&Toggle Layout\tF3");

    // Change the bitmaps for the Maximize item
    hBmp1 = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_CHECK1));
    hBmp2 = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_CHECK2));
    SetMenuItemBitmaps(hSysMenu, SC_MAXIMIZE, MF_BYCOMMAND, hBmp1, hBmp2);

    // Set the dialog's Small Icon
    hIcon = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_APP), IMAGE_ICON, 16, 16, 0);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

    // Set the dialog's Large Icon
    hIcon = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_APP), IMAGE_ICON, 32, 32, 0);
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

    // Create tooltips after all other windows
    g_hwndToolTip = CreateTooltip(hwnd);

    ForceVisibleDisplay(hwnd);

    // Set focus to first item
    return TRUE;
}

//
//  WM_NOTIFY handler
//
UINT WinSpyDlg_NotifyHandler(HWND hwnd, NMHDR *hdr)
{
    switch (hdr->code)
    {
        // TabView selection has changed, so show appropriate tab-pane
    case TCN_SELCHANGE:

        ShowWindow(WinSpyTab[nCurrentTab].hwnd, SW_HIDE);

        nCurrentTab = TabCtrl_GetCurSel(hdr->hwndFrom);

        SetWindowPos(WinSpyTab[nCurrentTab].hwnd, HWND_TOP, 0, 0, 0, 0, SWP_SHOWONLY);

        UpdateActiveTab();

        return TRUE;

        // TreeView has been right-clicked, so show the popup menu
    case NM_RCLICK:

        WindowTree_OnRightClick(hdr);

        return TRUE;

        // TreeView selection has changed, so update the main window properties
    case TVN_SELCHANGED:

        if (IsWindowVisible(GetDlgItem(hwnd, IDC_TREE1)))
        {
            WindowTree_OnSelectionChanged(hdr);
        }

        return TRUE;
    }

    return 0;
}

//
//  WM_SYSCOLORCHANGE handler
//
BOOL WinSpyDlg_SysColorChange(HWND hwnd)
{
    int i;

    // forward this to all the dialogs - they can look after
    // their own controls
    for (i = 0; i < NUMTABCONTROLITEMS; i++)
        PostMessage(WinSpyTab[i].hwnd, WM_SYSCOLORCHANGE, 0, 0);

    // Set the treeview colors
    TreeView_SetBkColor(GetDlgItem(hwnd, IDC_TREE1), GetSysColor(COLOR_WINDOW));

    // Recreate toolbar, so it uses new color scheme
    DestroyWindow(g_hwndPin);

    g_hwndPin = CreatePinToolbar(hwnd);

    // Send a WM_SIZE so that the pin toolbar gets repositioned
    SetWindowPos(hwnd, 0, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE |
        SWP_NOZORDER | SWP_NOACTIVATE |
        SWP_FRAMECHANGED);

    return TRUE;
}


//
// WM_HOTKEY handler
//
BOOL WinSpyDlg_OnHotKey(WPARAM id)
{
    if (id == HOTKEY_ID_SELECT_WINDOW_UNDER_CURSOR)
    {
        HWND hwnd;
        POINT pt;

        if (GetCursorPos(&pt))
        {
            // Open the window if collapsed.

            if (GetWindowLayout(g_hwndMain) == WINSPY_MINIMIZED)
            {
                SetWindowLayout(g_hwndMain, WINSPY_LASTMAX);
            }

            // Switch to the window under the mouse cursor.

            hwnd = WindowFromPointEx(pt, FALSE, FALSE);
            DisplayWindowInfo(hwnd);

            // Select the window in the treeview.

            if (GetWindowLayout(g_hwndMain) == WINSPY_EXPANDED)
            {
                WindowTree_Locate(hwnd);
            }
        }
    }

    return TRUE;
}


#ifdef _DEBUG
void DumpRect(HWND hwnd)
{
    RECT  rect;
    WCHAR ach[80];
    GetWindowRect(hwnd, &rect);
    swprintf_s(ach, ARRAYSIZE(ach), L"%d %d %d %d\n", rect.left, rect.top, rect.right, rect.bottom);
    OutputDebugString(ach);

}
#endif

//
//  Dialog procedure for main window
//
INT_PTR WINAPI DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WINDOWPLACEMENT placement;

    switch (msg)
    {
    case WM_INITDIALOG:
        return WinSpy_InitDlg(hwnd);

    case WM_CLOSE:
        ExitWinSpy(hwnd, 0);
        return TRUE;

    case WM_DESTROY:
        WindowTree_Destroy();
        return TRUE;

    case WM_SYSCOLORCHANGE:
        return WinSpyDlg_SysColorChange(hwnd);

    case WM_SYSCOMMAND:
        return WinSpyDlg_SysMenuHandler(hwnd, wParam, lParam);

    case WM_COMMAND:
        return WinSpyDlg_CommandHandler(hwnd, wParam, lParam);

    case WM_HOTKEY:
        return WinSpyDlg_OnHotKey(wParam);

    case WM_TIMER:
        return WinSpyDlg_TimerHandler(wParam);

    case WM_SIZE:
        return WinSpyDlg_Size(hwnd, wParam, lParam);

    case WM_SIZING:
        return WinSpyDlg_Sizing((UINT)wParam, (RECT *)lParam);

    case WM_NCHITTEST:
        return WinSpyDlg_NCHitTest(hwnd, wParam, lParam);

    case WM_ENTERSIZEMOVE:
        return WinSpyDlg_EnterSizeMove(hwnd);

    case WM_EXITSIZEMOVE:
        return WinSpyDlg_ExitSizeMove(hwnd);

    case WM_WINDOWPOSCHANGED:
        return WinSpyDlg_WindowPosChanged(hwnd, (WINDOWPOS *)lParam);

    case WM_NOTIFY:
        return WinSpyDlg_NotifyHandler(hwnd, (NMHDR *)lParam);

    case WM_DRAWITEM:
        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, DrawBitmapButton((DRAWITEMSTRUCT *)lParam));
        return TRUE;

        // Update our layout based on new settings
    case WM_SETTINGCHANGE:
        placement.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(hwnd, &placement);

        WinSpyDlg_SizeContents(hwnd);

        SetWindowPlacement(hwnd, &placement);
        return TRUE;
    }

    return FALSE;
}

//
// UpdateGlobalHotkey
//

BOOL g_fHotkeyRegistered = FALSE;
UINT g_hotkeyModifiers   = 0;
UINT g_hotkeyVKey        = 0;

void UpdateGlobalHotkey()
{
    // We can't have multiple instances of winspy registering for the same
    // global hotkey.  So, we only attempt anything related to the hotkey
    // in the first instance running.

    if (!g_fFirstInstance)
    {
        return;
    }

    UINT vkey      = 0;
    UINT modifiers = 0;

    // If enabled, sort out vkey and modifiers to use.

    if (g_opts.fEnableHotkey)
    {
        vkey = LOBYTE(g_opts.wHotkey);

        modifiers = MOD_WIN;

        if (HIBYTE(g_opts.wHotkey) & HOTKEYF_CONTROL)
            modifiers |= MOD_CONTROL;

        if (HIBYTE(g_opts.wHotkey) & HOTKEYF_ALT)
            modifiers |= MOD_ALT;

        if (HIBYTE(g_opts.wHotkey) & HOTKEYF_SHIFT)
            modifiers |= MOD_SHIFT;
    }

    // Has the configuration changed?

    BOOL fChanged = (g_opts.fEnableHotkey != g_fHotkeyRegistered) ||
                    (vkey != g_hotkeyVKey) ||
                    (modifiers != g_hotkeyModifiers);


    // Unregister the old one, if necessary.

    if (fChanged && g_fHotkeyRegistered)
    {
        UnregisterHotKey(g_hwndMain, HOTKEY_ID_SELECT_WINDOW_UNDER_CURSOR);

        g_fHotkeyRegistered = FALSE;
        g_hotkeyModifiers   = 0;
        g_hotkeyVKey        = 0;
    }

    // Register the new one, if necessary.

    if (fChanged && g_opts.fEnableHotkey)
    {
        if (RegisterHotKey(g_hwndMain, HOTKEY_ID_SELECT_WINDOW_UNDER_CURSOR, modifiers, vkey))
        {
            g_fHotkeyRegistered = TRUE;
            g_hotkeyModifiers   = modifiers;
            g_hotkeyVKey        = vkey;
        }
        else
        {
            g_opts.fEnableHotkey = FALSE;
            MessageBox(g_hwndMain, L"Failed to register hotkey.", szAppName, MB_OK | MB_ICONWARNING);
        }
    }
}


//
//  The only reason I do this is to "obfuscate" the main
//  window. All the windows are just dialogs (#32770), but
//  I use this function to make a new dialog class with
//  a different name..no other reason.
//
//  Check the dialog resources to see how the new name
//  is specified. (MFC extensions must be turned off for the resource
//  to enable this feature).
//
void RegisterDialogClass(WCHAR szNewName[])
{
    WNDCLASSEX wc;

    // Get the class structure for the system dialog class
    wc.cbSize = sizeof(wc);
    GetClassInfoEx(0, WC_DIALOG, &wc);

    // Make sure our new class does not conflict
    wc.style &= ~CS_GLOBALCLASS;

    // Register an identical dialog class, but with a new name!
    wc.lpszClassName = szNewName;

    RegisterClassEx(&wc);
}

PCSTR HelpBlurb =
    "Command Line Options:\n"
    "\n"
    "/pm\tRun in per-monitor DPI aware mode.\n"
    "/sa\tRun in system-aware DPI mode.\n"
    "\n";

BOOL ProcessCommandLine(PCSTR pcszCmdLine)
{
    BOOL fIsValid = TRUE;

    if (strcmp(pcszCmdLine, "/pm") == 0)
    {
        MarkProcessAsPerMonitorDpiAware();
    }
    else if (strcmp(pcszCmdLine, "/sa") == 0)
    {
        MarkProcessAsSystemDpiAware();
    }
    else if (pcszCmdLine[0])
    {
        MessageBoxA(NULL, HelpBlurb, "WinSpy", MB_OK);
        fIsValid = FALSE;
    }

    return fIsValid;
}

//
//  This is where the fun begins
//
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nShowCmd)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nShowCmd);
    HWND    hwndMain;
    HACCEL  hAccelTable;
    MSG     msg;

    if (!ProcessCommandLine(lpCmdLine))
    {
        return 0;
    }

    INITCOMMONCONTROLSEX ice;
    g_hInst = hInstance;

    ice.dwSize = sizeof ice;
    ice.dwICC = ICC_BAR_CLASSES | ICC_TREEVIEW_CLASSES |
        ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES;

    EnableDebugPrivilege();

    InitCommonControls();//Ex(&ice);

    RegisterDialogClass(L"WinSpy");
    RegisterDialogClass(L"WinSpyPane");

    // Remember if we are the first instance of winspy running.

    g_fFirstInstance = (FindWindow(L"WinSpy", NULL) == NULL);

    LoadSettings();

    //DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), 0, DialogProc);

    hwndMain = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAIN), 0, DialogProc);

    //Initialise the keyboard accelerators
    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));

    UpdateGlobalHotkey();

    //
    // UPDATED (fix for Matrox CenterPOPUP feature :)
    //
    //  If we use ShowWindow, then my Matrox card automatically centers WinSpy
    //  on the current monitor (even if we restored WinSpy to its position last
    //  time we ran). Therefore we use SetWindowPos to display the dialog, as
    //  Matrox don't seem to hook this in their display driver.
    //
    SetWindowPos(hwndMain, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        // Get the accelerator keys before IsDlgMsg gobbles them up!
        if (!TranslateAccelerator(hwndMain, hAccelTable, &msg))
        {
            // Let IsDialogMessage process TAB etc
            if (!IsDialogMessage(hwndMain, &msg) && !IsPosterMessage(&msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    SaveSettings();

    return 0;
}

void ExitWinSpy(HWND hwnd, UINT uCode)
{
    if (IsDlgButtonChecked(hwnd, IDC_AUTOUPDATE))
        KillTimer(hwnd, 0);

    DestroyWindow(hwnd);
    PostQuitMessage(uCode);
}
