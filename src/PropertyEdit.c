//
//  PropertyEdit.c
//
//  Copyright (c)
//  Freeware
//
//  Implements the Property Editor dialog box
//

#include "WinSpy.h"
#include "resource.h"
#include "Utils.h"

typedef struct
{
    HWND   hwndTarget;  // what window are we looking at??
    BOOL   bAddNew;

    WCHAR  szString[256];
    ATOM   aAtom;

} PropertyEditState;

INT_PTR CALLBACK PropertyEditProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    static PropertyEditState *state;

    HWND hwndList;

    HANDLE hHandle;
    WCHAR szText[32];

    switch (iMsg)
    {
    case WM_INITDIALOG:

        // Passed through in call to DialogBoxParam
        state = (PropertyEditState *)lParam;

        hwndList = GetDlgItem(hwnd, IDC_LIST1);

        if (state->bAddNew)
        {
            CheckRadioButton(hwnd, IDC_RADIO_NAME, IDC_RADIO_ATOM, IDC_RADIO_NAME);

            EnableWindow(GetDlgItem(hwnd, IDC_APPLY), FALSE);
        }
        else
        {
            if (state->aAtom)
            {
                CheckRadioButton(hwnd, IDC_RADIO_NAME, IDC_RADIO_ATOM, IDC_RADIO_ATOM);
                EnableWindow(GetDlgItem(hwnd, IDC_RADIO_NAME), FALSE);

                swprintf_s(szText, ARRAYSIZE(szText), L"%04hX", state->aAtom);
                SetDlgItemText(hwnd, IDC_EDIT_NAME, szText);

                hHandle = GetProp(state->hwndTarget, MAKEINTATOM(state->aAtom));
            }
            else
            {
                CheckRadioButton(hwnd, IDC_RADIO_NAME, IDC_RADIO_ATOM, IDC_RADIO_NAME);
                EnableWindow(GetDlgItem(hwnd, IDC_RADIO_ATOM), FALSE);

                SetDlgItemText(hwnd, IDC_EDIT_NAME, state->szString);

                hHandle = GetProp(state->hwndTarget, state->szString);
            }

            SendDlgItemMessage(hwnd, IDC_EDIT_NAME, EM_SETREADONLY, TRUE, 0);

            swprintf_s(szText, ARRAYSIZE(szText), L"%p", hHandle);
            SetDlgItemText(hwnd, IDC_EDIT_HANDLE, szText);
        }

        return TRUE;

    case WM_CLOSE:
        EndDialog(hwnd, 0);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_EDIT_NAME:
        case IDC_EDIT_HANDLE:

            if (HIWORD(wParam) == EN_CHANGE)
            {
                EnableWindow(
                    GetDlgItem(hwnd, IDC_APPLY),
                    SendDlgItemMessage(hwnd, IDC_EDIT_NAME, WM_GETTEXTLENGTH, 0, 0) > 0 &&
                    SendDlgItemMessage(hwnd, IDC_EDIT_HANDLE, WM_GETTEXTLENGTH, 0, 0) > 0
                );
            }

            return TRUE;

        case IDC_APPLY:

            hHandle = (HANDLE)GetDlgItemBaseInt(hwnd, IDC_EDIT_HANDLE, 16);

            if (IsDlgButtonChecked(hwnd, IDC_RADIO_ATOM))
            {
                state->aAtom = (ATOM)GetDlgItemBaseInt(hwnd, IDC_EDIT_NAME, 16);
                SetProp(state->hwndTarget, MAKEINTATOM(state->aAtom), hHandle);
            }
            else
            {
                GetDlgItemText(hwnd, IDC_EDIT_NAME, state->szString, 256);
                SetProp(state->hwndTarget, state->szString, hHandle);
            }

            return TRUE;

        case IDCANCEL:
            EndDialog(hwnd, 0);
            return TRUE;

        }
        return FALSE;
    }
    return FALSE;
}


void ShowWindowPropertyEditor(HWND hwndParent, HWND hwndTarget, BOOL bAddNew)
{
    HWND hwndList;
    LVITEM lvitem;
    PropertyEditState state;

    state.hwndTarget = hwndTarget;
    state.bAddNew = bAddNew;

    if (!bAddNew)
    {
        hwndList = GetDlgItem(hwndParent, IDC_LIST1);

        if (ListView_GetSelectedCount(hwndList) != 1)
            return;

        lvitem.mask = LVIF_TEXT | LVIF_PARAM;
        lvitem.iItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
        lvitem.iSubItem = 0;
        lvitem.pszText = state.szString;
        lvitem.cchTextMax = 256;

        ListView_GetItem(hwndList, &lvitem);

        state.aAtom = (ATOM)lvitem.lParam;
    }

    DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_PROPERTY_EDIT), hwndParent, PropertyEditProc, (LPARAM)&state);

    // Update the main display
    UpdatePropertyTab(hwndTarget);
}
