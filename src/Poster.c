//
//  Poster Dialog.
//
//  Copyright (c)
//  Freeware
//
//  Just a simple dialog which allows you to
//  send / post messages to a window
//

#include "WinSpy.h"

#include "resource.h"
#include "Utils.h"

typedef struct {
    const TCHAR *pszMsgName;
    UINT uMsgValue;
} MessageLookup;

// Based on WinUser.h
static const MessageLookup WindowMessages[] = {
    _T("WM_NULL"), 0x0000,
    _T("WM_CREATE"), 0x0001,
    _T("WM_DESTROY"), 0x0002,
    _T("WM_MOVE"), 0x0003,
    _T("WM_SIZE"), 0x0005,
    _T("WM_ACTIVATE"), 0x0006,
    _T("WM_SETFOCUS"), 0x0007,
    _T("WM_KILLFOCUS"), 0x0008,
    _T("WM_ENABLE"), 0x000A,
    _T("WM_SETREDRAW"), 0x000B,
    _T("WM_SETTEXT"), 0x000C,
    _T("WM_GETTEXT"), 0x000D,
    _T("WM_GETTEXTLENGTH"), 0x000E,
    _T("WM_PAINT"), 0x000F,
    _T("WM_CLOSE"), 0x0010,
    _T("WM_QUERYENDSESSION"), 0x0011,
    _T("WM_QUERYOPEN"), 0x0013,
    _T("WM_ENDSESSION"), 0x0016,
    _T("WM_QUIT"), 0x0012,
    _T("WM_ERASEBKGND"), 0x0014,
    _T("WM_SYSCOLORCHANGE"), 0x0015,
    _T("WM_SHOWWINDOW"), 0x0018,
    _T("WM_WININICHANGE"), 0x001A,
    _T("WM_SETTINGCHANGE"), 0x001A,
    _T("WM_DEVMODECHANGE"), 0x001B,
    _T("WM_ACTIVATEAPP"), 0x001C,
    _T("WM_FONTCHANGE"), 0x001D,
    _T("WM_TIMECHANGE"), 0x001E,
    _T("WM_CANCELMODE"), 0x001F,
    _T("WM_SETCURSOR"), 0x0020,
    _T("WM_MOUSEACTIVATE"), 0x0021,
    _T("WM_CHILDACTIVATE"), 0x0022,
    _T("WM_QUEUESYNC"), 0x0023,
    _T("WM_GETMINMAXINFO"), 0x0024,
    _T("WM_PAINTICON"), 0x0026,
    _T("WM_ICONERASEBKGND"), 0x0027,
    _T("WM_NEXTDLGCTL"), 0x0028,
    _T("WM_SPOOLERSTATUS"), 0x002A,
    _T("WM_DRAWITEM"), 0x002B,
    _T("WM_MEASUREITEM"), 0x002C,
    _T("WM_DELETEITEM"), 0x002D,
    _T("WM_VKEYTOITEM"), 0x002E,
    _T("WM_CHARTOITEM"), 0x002F,
    _T("WM_SETFONT"), 0x0030,
    _T("WM_GETFONT"), 0x0031,
    _T("WM_SETHOTKEY"), 0x0032,
    _T("WM_GETHOTKEY"), 0x0033,
    _T("WM_QUERYDRAGICON"), 0x0037,
    _T("WM_COMPAREITEM"), 0x0039,
    _T("WM_GETOBJECT"), 0x003D,
    _T("WM_COMPACTING"), 0x0041,
    _T("WM_COMMNOTIFY"), 0x0044,
    _T("WM_WINDOWPOSCHANGING"), 0x0046,
    _T("WM_WINDOWPOSCHANGED"), 0x0047,
    _T("WM_POWER"), 0x0048,
    _T("WM_COPYDATA"), 0x004A,
    _T("WM_CANCELJOURNAL"), 0x004B,
    _T("WM_NOTIFY"), 0x004E,
    _T("WM_INPUTLANGCHANGEREQUEST"), 0x0050,
    _T("WM_INPUTLANGCHANGE"), 0x0051,
    _T("WM_TCARD"), 0x0052,
    _T("WM_HELP"), 0x0053,
    _T("WM_USERCHANGED"), 0x0054,
    _T("WM_NOTIFYFORMAT"), 0x0055,
    _T("WM_CONTEXTMENU"), 0x007B,
    _T("WM_STYLECHANGING"), 0x007C,
    _T("WM_STYLECHANGED"), 0x007D,
    _T("WM_DISPLAYCHANGE"), 0x007E,
    _T("WM_GETICON"), 0x007F,
    _T("WM_SETICON"), 0x0080,
    _T("WM_NCCREATE"), 0x0081,
    _T("WM_NCDESTROY"), 0x0082,
    _T("WM_NCCALCSIZE"), 0x0083,
    _T("WM_NCHITTEST"), 0x0084,
    _T("WM_NCPAINT"), 0x0085,
    _T("WM_NCACTIVATE"), 0x0086,
    _T("WM_GETDLGCODE"), 0x0087,
    _T("WM_SYNCPAINT"), 0x0088,
    _T("WM_NCMOUSEMOVE"), 0x00A0,
    _T("WM_NCLBUTTONDOWN"), 0x00A1,
    _T("WM_NCLBUTTONUP"), 0x00A2,
    _T("WM_NCLBUTTONDBLCLK"), 0x00A3,
    _T("WM_NCRBUTTONDOWN"), 0x00A4,
    _T("WM_NCRBUTTONUP"), 0x00A5,
    _T("WM_NCRBUTTONDBLCLK"), 0x00A6,
    _T("WM_NCMBUTTONDOWN"), 0x00A7,
    _T("WM_NCMBUTTONUP"), 0x00A8,
    _T("WM_NCMBUTTONDBLCLK"), 0x00A9,
    _T("WM_NCXBUTTONDOWN"), 0x00AB,
    _T("WM_NCXBUTTONUP"), 0x00AC,
    _T("WM_NCXBUTTONDBLCLK"), 0x00AD,
    _T("WM_INPUT_DEVICE_CHANGE"), 0x00FE,
    _T("WM_INPUT"), 0x00FF,
    _T("WM_KEYFIRST"), 0x0100,
    _T("WM_KEYDOWN"), 0x0100,
    _T("WM_KEYUP"), 0x0101,
    _T("WM_CHAR"), 0x0102,
    _T("WM_DEADCHAR"), 0x0103,
    _T("WM_SYSKEYDOWN"), 0x0104,
    _T("WM_SYSKEYUP"), 0x0105,
    _T("WM_SYSCHAR"), 0x0106,
    _T("WM_SYSDEADCHAR"), 0x0107,
    _T("WM_UNICHAR"), 0x0109,
    _T("WM_KEYLAST"), 0x0109,
    _T("WM_KEYLAST"), 0x0108,
    _T("WM_IME_STARTCOMPOSITION"), 0x010D,
    _T("WM_IME_ENDCOMPOSITION"), 0x010E,
    _T("WM_IME_COMPOSITION"), 0x010F,
    _T("WM_IME_KEYLAST"), 0x010F,
    _T("WM_INITDIALOG"), 0x0110,
    _T("WM_COMMAND"), 0x0111,
    _T("WM_SYSCOMMAND"), 0x0112,
    _T("WM_TIMER"), 0x0113,
    _T("WM_HSCROLL"), 0x0114,
    _T("WM_VSCROLL"), 0x0115,
    _T("WM_INITMENU"), 0x0116,
    _T("WM_INITMENUPOPUP"), 0x0117,
    _T("WM_GESTURE"), 0x0119,
    _T("WM_GESTURENOTIFY"), 0x011A,
    _T("WM_MENUSELECT"), 0x011F,
    _T("WM_MENUCHAR"), 0x0120,
    _T("WM_ENTERIDLE"), 0x0121,
    _T("WM_MENURBUTTONUP"), 0x0122,
    _T("WM_MENUDRAG"), 0x0123,
    _T("WM_MENUGETOBJECT"), 0x0124,
    _T("WM_UNINITMENUPOPUP"), 0x0125,
    _T("WM_MENUCOMMAND"), 0x0126,
    _T("WM_CHANGEUISTATE"), 0x0127,
    _T("WM_UPDATEUISTATE"), 0x0128,
    _T("WM_QUERYUISTATE"), 0x0129,
    _T("WM_CTLCOLORMSGBOX"), 0x0132,
    _T("WM_CTLCOLOREDIT"), 0x0133,
    _T("WM_CTLCOLORLISTBOX"), 0x0134,
    _T("WM_CTLCOLORBTN"), 0x0135,
    _T("WM_CTLCOLORDLG"), 0x0136,
    _T("WM_CTLCOLORSCROLLBAR"), 0x0137,
    _T("WM_CTLCOLORSTATIC"), 0x0138,
    _T("MN_GETHMENU"), 0x01E1,
    _T("WM_MOUSEFIRST"), 0x0200,
    _T("WM_MOUSEMOVE"), 0x0200,
    _T("WM_LBUTTONDOWN"), 0x0201,
    _T("WM_LBUTTONUP"), 0x0202,
    _T("WM_LBUTTONDBLCLK"), 0x0203,
    _T("WM_RBUTTONDOWN"), 0x0204,
    _T("WM_RBUTTONUP"), 0x0205,
    _T("WM_RBUTTONDBLCLK"), 0x0206,
    _T("WM_MBUTTONDOWN"), 0x0207,
    _T("WM_MBUTTONUP"), 0x0208,
    _T("WM_MBUTTONDBLCLK"), 0x0209,
    _T("WM_MOUSEWHEEL"), 0x020A,
    _T("WM_XBUTTONDOWN"), 0x020B,
    _T("WM_XBUTTONUP"), 0x020C,
    _T("WM_XBUTTONDBLCLK"), 0x020D,
    _T("WM_MOUSEHWHEEL"), 0x020E,
    _T("WM_MOUSELAST"), 0x020E,
    _T("WM_MOUSELAST"), 0x020D,
    _T("WM_MOUSELAST"), 0x020A,
    _T("WM_MOUSELAST"), 0x0209,
    _T("WM_PARENTNOTIFY"), 0x0210,
    _T("WM_ENTERMENULOOP"), 0x0211,
    _T("WM_EXITMENULOOP"), 0x0212,
    _T("WM_NEXTMENU"), 0x0213,
    _T("WM_SIZING"), 0x0214,
    _T("WM_CAPTURECHANGED"), 0x0215,
    _T("WM_MOVING"), 0x0216,
    _T("WM_POWERBROADCAST"), 0x0218,
    _T("WM_DEVICECHANGE"), 0x0219,
    _T("WM_MDICREATE"), 0x0220,
    _T("WM_MDIDESTROY"), 0x0221,
    _T("WM_MDIACTIVATE"), 0x0222,
    _T("WM_MDIRESTORE"), 0x0223,
    _T("WM_MDINEXT"), 0x0224,
    _T("WM_MDIMAXIMIZE"), 0x0225,
    _T("WM_MDITILE"), 0x0226,
    _T("WM_MDICASCADE"), 0x0227,
    _T("WM_MDIICONARRANGE"), 0x0228,
    _T("WM_MDIGETACTIVE"), 0x0229,
    _T("WM_MDISETMENU"), 0x0230,
    _T("WM_ENTERSIZEMOVE"), 0x0231,
    _T("WM_EXITSIZEMOVE"), 0x0232,
    _T("WM_DROPFILES"), 0x0233,
    _T("WM_MDIREFRESHMENU"), 0x0234,
    _T("WM_TOUCH"), 0x0240,
    _T("WM_IME_SETCONTEXT"), 0x0281,
    _T("WM_IME_NOTIFY"), 0x0282,
    _T("WM_IME_CONTROL"), 0x0283,
    _T("WM_IME_COMPOSITIONFULL"), 0x0284,
    _T("WM_IME_SELECT"), 0x0285,
    _T("WM_IME_CHAR"), 0x0286,
    _T("WM_IME_REQUEST"), 0x0288,
    _T("WM_IME_KEYDOWN"), 0x0290,
    _T("WM_IME_KEYUP"), 0x0291,
    _T("WM_MOUSEHOVER"), 0x02A1,
    _T("WM_MOUSELEAVE"), 0x02A3,
    _T("WM_NCMOUSEHOVER"), 0x02A0,
    _T("WM_NCMOUSELEAVE"), 0x02A2,
    _T("WM_WTSSESSION_CHANGE"), 0x02B1,
    _T("WM_TABLET_FIRST"), 0x02c0,
    _T("WM_TABLET_LAST"), 0x02df,
    _T("WM_CUT"), 0x0300,
    _T("WM_COPY"), 0x0301,
    _T("WM_PASTE"), 0x0302,
    _T("WM_CLEAR"), 0x0303,
    _T("WM_UNDO"), 0x0304,
    _T("WM_RENDERFORMAT"), 0x0305,
    _T("WM_RENDERALLFORMATS"), 0x0306,
    _T("WM_DESTROYCLIPBOARD"), 0x0307,
    _T("WM_DRAWCLIPBOARD"), 0x0308,
    _T("WM_PAINTCLIPBOARD"), 0x0309,
    _T("WM_VSCROLLCLIPBOARD"), 0x030A,
    _T("WM_SIZECLIPBOARD"), 0x030B,
    _T("WM_ASKCBFORMATNAME"), 0x030C,
    _T("WM_CHANGECBCHAIN"), 0x030D,
    _T("WM_HSCROLLCLIPBOARD"), 0x030E,
    _T("WM_QUERYNEWPALETTE"), 0x030F,
    _T("WM_PALETTEISCHANGING"), 0x0310,
    _T("WM_PALETTECHANGED"), 0x0311,
    _T("WM_HOTKEY"), 0x0312,
    _T("WM_PRINT"), 0x0317,
    _T("WM_PRINTCLIENT"), 0x0318,
    _T("WM_APPCOMMAND"), 0x0319,
    _T("WM_THEMECHANGED"), 0x031A,
    _T("WM_CLIPBOARDUPDATE"), 0x031D,
    _T("WM_DWMCOMPOSITIONCHANGED"), 0x031E,
    _T("WM_DWMNCRENDERINGCHANGED"), 0x031F,
    _T("WM_DWMCOLORIZATIONCOLORCHANGED"), 0x0320,
    _T("WM_DWMWINDOWMAXIMIZEDCHANGE"), 0x0321,
    _T("WM_DWMSENDICONICTHUMBNAIL"), 0x0323,
    _T("WM_DWMSENDICONICLIVEPREVIEWBITMAP"), 0x0326,
    _T("WM_GETTITLEBARINFOEX"), 0x033F,
    _T("WM_HANDHELDFIRST"), 0x0358,
    _T("WM_HANDHELDLAST"), 0x035F,
    _T("WM_AFXFIRST"), 0x0360,
    _T("WM_AFXLAST"), 0x037F,
    _T("WM_PENWINFIRST"), 0x0380,
    _T("WM_PENWINLAST"), 0x038F,
    _T("WM_APP"), 0x8000,
    _T("WM_USER"), 0x0400,
    _T("EM_GETSEL"), 0x00B0,
    _T("EM_SETSEL"), 0x00B1,
    _T("EM_GETRECT"), 0x00B2,
    _T("EM_SETRECT"), 0x00B3,
    _T("EM_SETRECTNP"), 0x00B4,
    _T("EM_SCROLL"), 0x00B5,
    _T("EM_LINESCROLL"), 0x00B6,
    _T("EM_SCROLLCARET"), 0x00B7,
    _T("EM_GETMODIFY"), 0x00B8,
    _T("EM_SETMODIFY"), 0x00B9,
    _T("EM_GETLINECOUNT"), 0x00BA,
    _T("EM_LINEINDEX"), 0x00BB,
    _T("EM_SETHANDLE"), 0x00BC,
    _T("EM_GETHANDLE"), 0x00BD,
    _T("EM_GETTHUMB"), 0x00BE,
    _T("EM_LINELENGTH"), 0x00C1,
    _T("EM_REPLACESEL"), 0x00C2,
    _T("EM_GETLINE"), 0x00C4,
    _T("EM_LIMITTEXT"), 0x00C5,
    _T("EM_CANUNDO"), 0x00C6,
    _T("EM_UNDO"), 0x00C7,
    _T("EM_FMTLINES"), 0x00C8,
    _T("EM_LINEFROMCHAR"), 0x00C9,
    _T("EM_SETTABSTOPS"), 0x00CB,
    _T("EM_SETPASSWORDCHAR"), 0x00CC,
    _T("EM_EMPTYUNDOBUFFER"), 0x00CD,
    _T("EM_GETFIRSTVISIBLELINE"), 0x00CE,
    _T("EM_SETREADONLY"), 0x00CF,
    _T("EM_SETWORDBREAKPROC"), 0x00D0,
    _T("EM_GETWORDBREAKPROC"), 0x00D1,
    _T("EM_GETPASSWORDCHAR"), 0x00D2,
    _T("EM_SETMARGINS"), 0x00D3,
    _T("EM_GETMARGINS"), 0x00D4,
    _T("EM_SETLIMITTEXT"), 0x00C5,
    _T("EM_GETLIMITTEXT"), 0x00D5,
    _T("EM_POSFROMCHAR"), 0x00D6,
    _T("EM_CHARFROMPOS"), 0x00D7,
    _T("EM_SETIMESTATUS"), 0x00D8,
    _T("EM_GETIMESTATUS"), 0x00D9,
    _T("BM_GETCHECK"), 0x00F0,
    _T("BM_SETCHECK"), 0x00F1,
    _T("BM_GETSTATE"), 0x00F2,
    _T("BM_SETSTATE"), 0x00F3,
    _T("BM_SETSTYLE"), 0x00F4,
    _T("BM_CLICK"), 0x00F5,
    _T("BM_GETIMAGE"), 0x00F6,
    _T("BM_SETIMAGE"), 0x00F7,
    _T("BM_SETDONTCLICK"), 0x00F8,
    _T("STM_SETICON"), 0x0170,
    _T("STM_GETICON"), 0x0171,
    _T("STM_SETIMAGE"), 0x0172,
    _T("STM_GETIMAGE"), 0x0173,
    _T("STM_MSGMAX"), 0x0174,
    _T("LB_ADDSTRING"), 0x0180,
    _T("LB_INSERTSTRING"), 0x0181,
    _T("LB_DELETESTRING"), 0x0182,
    _T("LB_SELITEMRANGEEX"), 0x0183,
    _T("LB_RESETCONTENT"), 0x0184,
    _T("LB_SETSEL"), 0x0185,
    _T("LB_SETCURSEL"), 0x0186,
    _T("LB_GETSEL"), 0x0187,
    _T("LB_GETCURSEL"), 0x0188,
    _T("LB_GETTEXT"), 0x0189,
    _T("LB_GETTEXTLEN"), 0x018A,
    _T("LB_GETCOUNT"), 0x018B,
    _T("LB_SELECTSTRING"), 0x018C,
    _T("LB_DIR"), 0x018D,
    _T("LB_GETTOPINDEX"), 0x018E,
    _T("LB_FINDSTRING"), 0x018F,
    _T("LB_GETSELCOUNT"), 0x0190,
    _T("LB_GETSELITEMS"), 0x0191,
    _T("LB_SETTABSTOPS"), 0x0192,
    _T("LB_GETHORIZONTALEXTENT"), 0x0193,
    _T("LB_SETHORIZONTALEXTENT"), 0x0194,
    _T("LB_SETCOLUMNWIDTH"), 0x0195,
    _T("LB_ADDFILE"), 0x0196,
    _T("LB_SETTOPINDEX"), 0x0197,
    _T("LB_GETITEMRECT"), 0x0198,
    _T("LB_GETITEMDATA"), 0x0199,
    _T("LB_SETITEMDATA"), 0x019A,
    _T("LB_SELITEMRANGE"), 0x019B,
    _T("LB_SETANCHORINDEX"), 0x019C,
    _T("LB_GETANCHORINDEX"), 0x019D,
    _T("LB_SETCARETINDEX"), 0x019E,
    _T("LB_GETCARETINDEX"), 0x019F,
    _T("LB_SETITEMHEIGHT"), 0x01A0,
    _T("LB_GETITEMHEIGHT"), 0x01A1,
    _T("LB_FINDSTRINGEXACT"), 0x01A2,
    _T("LB_SETLOCALE"), 0x01A5,
    _T("LB_GETLOCALE"), 0x01A6,
    _T("LB_SETCOUNT"), 0x01A7,
    _T("LB_INITSTORAGE"), 0x01A8,
    _T("LB_ITEMFROMPOINT"), 0x01A9,
    _T("LB_MULTIPLEADDSTRING"), 0x01B1,
    _T("LB_GETLISTBOXINFO"), 0x01B2,
    _T("LB_MSGMAX"), 0x01B3,
    _T("LB_MSGMAX"), 0x01B1,
    _T("LB_MSGMAX"), 0x01B0,
    _T("LB_MSGMAX"), 0x01A8,
    _T("CB_GETEDITSEL"), 0x0140,
    _T("CB_LIMITTEXT"), 0x0141,
    _T("CB_SETEDITSEL"), 0x0142,
    _T("CB_ADDSTRING"), 0x0143,
    _T("CB_DELETESTRING"), 0x0144,
    _T("CB_DIR"), 0x0145,
    _T("CB_GETCOUNT"), 0x0146,
    _T("CB_GETCURSEL"), 0x0147,
    _T("CB_GETLBTEXT"), 0x0148,
    _T("CB_GETLBTEXTLEN"), 0x0149,
    _T("CB_INSERTSTRING"), 0x014A,
    _T("CB_RESETCONTENT"), 0x014B,
    _T("CB_FINDSTRING"), 0x014C,
    _T("CB_SELECTSTRING"), 0x014D,
    _T("CB_SETCURSEL"), 0x014E,
    _T("CB_SHOWDROPDOWN"), 0x014F,
    _T("CB_GETITEMDATA"), 0x0150,
    _T("CB_SETITEMDATA"), 0x0151,
    _T("CB_GETDROPPEDCONTROLRECT"), 0x0152,
    _T("CB_SETITEMHEIGHT"), 0x0153,
    _T("CB_GETITEMHEIGHT"), 0x0154,
    _T("CB_SETEXTENDEDUI"), 0x0155,
    _T("CB_GETEXTENDEDUI"), 0x0156,
    _T("CB_GETDROPPEDSTATE"), 0x0157,
    _T("CB_FINDSTRINGEXACT"), 0x0158,
    _T("CB_SETLOCALE"), 0x0159,
    _T("CB_GETLOCALE"), 0x015A,
    _T("CB_GETTOPINDEX"), 0x015b,
    _T("CB_SETTOPINDEX"), 0x015c,
    _T("CB_GETHORIZONTALEXTENT"), 0x015d,
    _T("CB_SETHORIZONTALEXTENT"), 0x015e,
    _T("CB_GETDROPPEDWIDTH"), 0x015f,
    _T("CB_SETDROPPEDWIDTH"), 0x0160,
    _T("CB_INITSTORAGE"), 0x0161,
    _T("CB_MULTIPLEADDSTRING"), 0x0163,
    _T("CB_GETCOMBOBOXINFO"), 0x0164,
    _T("CB_MSGMAX"), 0x0165,
    _T("CB_MSGMAX"), 0x0163,
    _T("CB_MSGMAX"), 0x0162,
    _T("CB_MSGMAX"), 0x015B,
    _T("SBM_SETPOS"), 0x00E0,
    _T("SBM_GETPOS"), 0x00E1,
    _T("SBM_SETRANGE"), 0x00E2,
    _T("SBM_SETRANGEREDRAW"), 0x00E6,
    _T("SBM_GETRANGE"), 0x00E3,
    _T("SBM_ENABLE_ARROWS"), 0x00E4,
    _T("SBM_SETSCROLLINFO"), 0x00E9,
    _T("SBM_GETSCROLLINFO"), 0x00EA,
    _T("SBM_GETSCROLLBARINFO"), 0x00EB,
    NULL, 0
};

//
// Checks if window is valid for Poster
//
static BOOL IsWindowEx(HWND hwnd) 
{
    return hwnd == HWND_BROADCAST || IsWindow(hwnd);
}

static void SetInitialGuiInfo(HWND hwnd, HWND hwndTarget)
{
    HWND     hwndMsgsCombo;
    TCHAR    ach[256];

    _stprintf_s(ach, ARRAYSIZE(ach), szHexFmt, (UINT)(UINT_PTR)hwndTarget);
    SetDlgItemText(hwnd, IDC_POSTER_HANDLE, ach);

    hwndMsgsCombo = GetDlgItem(hwnd, IDC_POSTER_MESSAGES);
    for (int i = 0; WindowMessages[i].pszMsgName; i++)
    {
        int index = (int)SendMessage(hwndMsgsCombo, CB_ADDSTRING, 0, (LPARAM)WindowMessages[i].pszMsgName);
        if (index != -1)
        {
            SendMessage(hwndMsgsCombo, CB_SETITEMDATA, index, WindowMessages[i].uMsgValue);
        }
    }
}

static void GetGuiInfo(HWND hwnd, HWND *phwndTarget, UINT *puMsg, WPARAM *pwParam, LPARAM *plParam)
{
    HWND     hwndMsgsCombo;
    int      nComboIndex;

    *phwndTarget = (HWND)GetDlgItemBaseInt(hwnd, IDC_POSTER_HANDLE, 16);

    hwndMsgsCombo = GetDlgItem(hwnd, IDC_POSTER_MESSAGES);
    nComboIndex = (int)SendMessage(hwndMsgsCombo, CB_GETCURSEL, 0, 0);
    if (nComboIndex != -1)
    {
        *puMsg = (UINT)SendMessage(hwndMsgsCombo, CB_GETITEMDATA, nComboIndex, 0);
    }
    else
    {
        *puMsg = (UINT)GetDlgItemBaseInt(hwnd, IDC_POSTER_MESSAGES, 16);
    }

    *pwParam = (WPARAM)GetDlgItemBaseInt(hwnd, IDC_POSTER_WPARAM, 16);
    *plParam = (LPARAM)GetDlgItemBaseInt(hwnd, IDC_POSTER_LPARAM, 16);
}

static void PosterSendMessage(HWND hwnd)
{
    HWND     hwndTarget;
    UINT     uMsg;
    WPARAM   wParam;
    LPARAM   lParam;
    TCHAR    ach[256];
    DWORD_PTR dwResult;

    GetGuiInfo(hwnd, &hwndTarget, &uMsg, &wParam, &lParam);

    if (!IsWindowEx(hwndTarget))
    {
        MessageBox(hwnd,
            _T("Not a valid window"),
            szAppName,
            MB_OK | MB_ICONEXCLAMATION);
        return;
    }

    if (SendMessageTimeout(hwndTarget, uMsg, wParam, lParam, 0, 7000, &dwResult))
    {
        _stprintf_s(ach, ARRAYSIZE(ach), szPtrFmt, (void*)dwResult);
    }
    else
    {
        _stprintf_s(ach, ARRAYSIZE(ach), _T("Error 0x") szHexFmt, GetLastError());
    }

    SetDlgItemText(hwnd, IDC_POSTER_RESULT, ach);
}

static void PosterPostMessage(HWND hwnd)
{
    HWND     hwndTarget;
    UINT     uMsg;
    WPARAM   wParam;
    LPARAM   lParam;
    TCHAR    ach[256];

    GetGuiInfo(hwnd, &hwndTarget, &uMsg, &wParam, &lParam);

    if (!IsWindowEx(hwndTarget))
    {
        MessageBox(hwnd,
            _T("Not a valid window"),
            szAppName,
            MB_OK | MB_ICONEXCLAMATION);
        return;
    }

    if (PostMessage(hwndTarget, uMsg, wParam, lParam))
    {
        _tcscpy_s(ach, ARRAYSIZE(ach), _T("Posted"));
    }
    else
    {
        _stprintf_s(ach, ARRAYSIZE(ach), _T("Error 0x") szHexFmt, GetLastError());
    }

    SetDlgItemText(hwnd, IDC_POSTER_RESULT, ach);
}

//
//  Dialog procedure for the poster window
//
INT_PTR CALLBACK PosterDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndTarget;    // target window!

    switch (iMsg)
    {
    case WM_INITDIALOG:
        hwndTarget = (HWND)lParam;

        SetInitialGuiInfo(hwnd, hwndTarget);
        return TRUE;

    case WM_CLOSE:
        EndDialog(hwnd, 0);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_POSTER_SEND:
            PosterSendMessage(hwnd);
            return TRUE;

        case IDC_POSTER_POST:
            PosterPostMessage(hwnd);
            return TRUE;

        case IDCANCEL:
            EndDialog(hwnd, 0);
            return TRUE;
        }
        return FALSE;
    }

    return FALSE;
}


void ShowPosterDlg(HWND hwndParent, HWND hwndTarget)
{
    if (IsWindow(spy_hCurWnd))
    {
        DialogBoxParam(
            hInst,
            MAKEINTRESOURCE(IDD_POSTER),
            hwndParent,
            PosterDlgProc,
            (LPARAM)hwndTarget);

        SetGeneralInfo(hwndTarget);
    }
    else
    {
        MessageBox(hwndParent,
            _T("Not a valid window"),
            szAppName,
            MB_OK | MB_ICONEXCLAMATION);
    }
}


void ShowBroadcasterDlg(HWND hwndParent)
{
    DialogBoxParam(
        hInst,
        MAKEINTRESOURCE(IDD_POSTER),
        hwndParent,
        PosterDlgProc,
        (LPARAM)HWND_BROADCAST);
}

