//
//  AutoCompleteComboBox.c
//
//  To implement an autocomplete in ComboBox
//


#include "WinSpy.h"
#include <tchar.h>


#define ACC_EDIT_PROC TEXT("Acc_Edit_Proc")
#define GetEditOriProc(hEdit) (WNDPROC)GetProp(hEdit, ACC_EDIT_PROC)


static void ACComboBoxEdit_AutoComplete(HWND hComboBox, TCHAR ch)
{
    TCHAR txt[MAX_PATH] = { 0 };
    int index = ComboBox_GetCurSel(hComboBox);

    if (!_istcntrl(ch))
    {
        ComboBox_GetText(hComboBox, txt, ARRAYSIZE(txt));

        index = ComboBox_FindStringExact(hComboBox, index, txt);
        if (CB_ERR == index)
        {
            index = ComboBox_FindString(hComboBox, index, txt);
        }

        if (CB_ERR != index)
        {
            ComboBox_SetCurSel(hComboBox, index);
            ComboBox_SetEditSel(hComboBox, _tcslen(txt), -1);
        }
    }
}

//
// Paste text cannot trigger autocomplete, so when the control
// loses focus, we will try to implement it manually.
//
static void ACComboBoxEdit_OnKillFocus(HWND hComboBox)
{
    TCHAR txt[MAX_PATH] = { 0 };

    int tc = ComboBox_GetText(hComboBox, txt, ARRAYSIZE(txt));
    int si = ComboBox_GetCurSel(hComboBox);
    int ei = ComboBox_FindStringExact(hComboBox, si, txt);

    if (tc != 0 && CB_ERR != ei && si != ei)
    {
        ComboBox_SetCurSel(hComboBox, ei);
    }
}

static LRESULT CALLBACK ACComboBoxEdit_Proc(HWND hEdit, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CHAR:
    {
        LRESULT r = CallWindowProc(GetEditOriProc(hEdit), hEdit, msg, wParam, lParam);
        ACComboBoxEdit_AutoComplete(GetParent(hEdit), (TCHAR)wParam);
        return r;
    }

    case WM_KILLFOCUS:
        ACComboBoxEdit_OnKillFocus(GetParent(hEdit));
        break;

    case WM_DESTROY:
        SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)GetEditOriProc(hEdit));
        RemoveProp(hEdit, ACC_EDIT_PROC);
        break;
    }

    return CallWindowProc(GetEditOriProc(hEdit), hEdit, msg, wParam, lParam);
}

static void ACComboBoxEdit_Init(HWND hComboBox)
{
    HWND     h = FindWindowEx(hComboBox, NULL, WC_EDIT, NULL);
    LONG_PTR o = SetWindowLongPtr(h, GWLP_WNDPROC, (LONG_PTR)ACComboBoxEdit_Proc);

    SetProp(h, ACC_EDIT_PROC, (HANDLE)o);

    ComboBox_LimitText(hComboBox, MAX_PATH - 1);
}

void InitAutoCompleteComboBox(HWND hDlg, int nIdComboBox)
{
    TCHAR txt[MAX_PATH] = { 0 };
    HWND hcb = GetDlgItem(hDlg, nIdComboBox);

    GetClassName(hcb, txt, ARRAYSIZE(txt));

    if (lstrcmp(WC_COMBOBOX, txt) == 0)
    {
        ACComboBoxEdit_Init(hcb);
    }
}
