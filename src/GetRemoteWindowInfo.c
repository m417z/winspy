//
//  GetRemoteClassInfoEx.c
//
//  Copyright (c) 2002 by J Brown
//  Freeware
//
//  In order to retrieve private class information for a
//  window in another process, we have to create
//  a remote thread in that process and call GetClassInfoEx from
//  there.
//
//  GetRemoteClassInfoEx uses the InjectRemoteThread call defined
//  in InjectThread.c
//

#include "WinSpy.h"

#include <psapi.h>
#include "InjectThread.h"

typedef BOOL(WINAPI *PROCGETCLASSINFOEXW)(HINSTANCE, PCWSTR, WNDCLASSEXW*);
typedef LONG_PTR(WINAPI *PROCGETWINDOWLONGPTR)(HWND, int);
typedef LRESULT(WINAPI *PROCSENDMESSAGETO)(HWND, UINT, WPARAM, LPARAM, UINT, UINT, PDWORD_PTR);

//
//  Define a structure for the remote thread to use
//
typedef struct
{
    // Input starts
    PROCGETCLASSINFOEXW   fnGetClassInfoEx;
    PROCGETWINDOWLONGPTR  fnGetWindowLongPtr;
    PROCSENDMESSAGETO     fnSendMessageTimeout;

    HWND        hwnd; //window we want to get class info for
    ATOM        atom; //class atom of window
    HINSTANCE   hInst;
    int         nTextSize;

    // Output starts
    WNDCLASSEXW wcOutput;
    WNDPROC     wndproc;

    WCHAR       szText[200]; // Window text to retrieve
} INJDATA;

#pragma runtime_checks("", off)
// calls to the stack checking routine must be disabled
#pragma check_stack(off)

// From https://msdn.microsoft.com/en-us/library/7977wcck.aspx:
// The order here is important.
// Section names must be 8 characters or less.
// The sections with the same name before the $
// are merged into one section. The order that
// they are merged is determined by sorting
// the characters after the $.
// InitSegStart and InitSegEnd are used to set
// boundaries so we can find the real functions
// that we need to call for initialization.

//
//  Thread to inject to remote process. Must not
//  make ANY calls to code in THIS process.
//
__declspec(code_seg(".inject$a"))
// GetDataProc will selectively collect 3 pieces of information about the target window:
// - window procedure
// - window class
// - text
static DWORD WINAPI GetDataProc(PVOID pParam)
{
    INJDATA *pInjData = (INJDATA *)pParam;
    BOOL    fRet = TRUE;
    DWORD_PTR dwpResult;

    if (pInjData->fnGetWindowLongPtr)
        pInjData->wndproc = (WNDPROC)pInjData->fnGetWindowLongPtr(pInjData->hwnd, GWLP_WNDPROC);

    if (pInjData->fnGetClassInfoEx)
    {
        static_assert(sizeof(pInjData->wcOutput) == sizeof(WNDCLASSEXA), "Unicode and ANSI structures expected to be the same size");
        pInjData->wcOutput.cbSize = sizeof(pInjData->wcOutput);
        fRet = fRet && pInjData->fnGetClassInfoEx(pInjData->hInst, (PCWSTR)(intptr_t)pInjData->atom, &pInjData->wcOutput);
    }

    if (pInjData->fnSendMessageTimeout)
    {
        // Null-terminate in case the gettext fails
        pInjData->szText[0] = L'\0';

        pInjData->fnSendMessageTimeout(pInjData->hwnd, WM_GETTEXT,
            pInjData->nTextSize, (LPARAM)pInjData->szText,
            SMTO_ABORTIFHUNG, 100, &dwpResult);
    }

    return fRet;

}

__declspec(code_seg(".inject$z"))
static void AfterGetDataProc(void) { }

#pragma check_stack
#pragma runtime_checks("", restore)

BOOL IsInsideModule(MODULEINFO *pModuleInfo, PVOID fn)
{
    return !fn || (pModuleInfo->lpBaseOfDll <= fn && (PVOID)((BYTE*)pModuleInfo->lpBaseOfDll + pModuleInfo->SizeOfImage) > fn);
}

BOOL IsInjectionDataValid(INJDATA *pInjData)
{
    // It is only safe to inject this code if we are passing the addresses of functions in user32.dll (which is shared across all processes).
    // If an appcompat shim is applied to our process that replaces any of these functions' pointers with ones outside of user32.dll,
    // we cannot really do anything about this (as it will also override the GetProcAddress behavior), so the best we can do is fail gracefully
    HMODULE hModUser32 = GetModuleHandle(L"user32.dll");
    if (!hModUser32)
        return FALSE;

    MODULEINFO moduleInfo;
    if (!GetModuleInformation(GetCurrentProcess(), hModUser32, &moduleInfo, sizeof(moduleInfo)))
        return FALSE;

    return (IsInsideModule(&moduleInfo, (PVOID)(intptr_t)pInjData->fnSendMessageTimeout) &&
        IsInsideModule(&moduleInfo, (PVOID)(intptr_t)pInjData->fnGetWindowLongPtr) &&
        IsInsideModule(&moduleInfo, (PVOID)(intptr_t)pInjData->fnGetClassInfoEx));
}

BOOL GetRemoteWindowInfo(HWND hwnd, WNDCLASSEX *pClass, WNDPROC *pProc, WCHAR *pszText, int nTextLen)
{
    INJDATA InjData;
    BOOL    fReturn;

    // Calculate how many bytes the injected code takes
    DWORD_PTR cbCodeSize = ((BYTE *)(intptr_t)AfterGetDataProc - (BYTE *)(intptr_t)GetDataProc);

    //
    // Setup the injection structure:
    //
    ZeroMemory(&InjData, sizeof(InjData));

    // Get pointers to the API calls we will be using in the remote thread
    if (pszText)
        InjData.fnSendMessageTimeout = SendMessageTimeout;
    if (pProc)
        InjData.fnGetWindowLongPtr = IsWindowUnicode(hwnd) ? GetWindowLongPtrW : GetWindowLongPtrA;
    if (pClass)
        InjData.fnGetClassInfoEx = IsWindowUnicode(hwnd) ? GetClassInfoExW : (PROCGETCLASSINFOEXW)GetClassInfoExA;

    // Setup the data the API calls will need
    InjData.hwnd = (HWND)hwnd;
    InjData.atom = (ATOM)GetClassLong(hwnd, GCW_ATOM);
    InjData.hInst = (HINSTANCE)GetClassLongPtr(hwnd, GCLP_HMODULE);
    InjData.nTextSize = ARRAYSIZE(InjData.szText);
    InjData.wndproc = 0;

    //
    // Inject the GetClassInfoExProc function, and our InjData structure!
    //
#define offsetof(s,m) ((size_t)&(((s*)0)->m))
    fReturn = IsInjectionDataValid(&InjData) && InjectRemoteThread(hwnd, GetDataProc, cbCodeSize, &InjData, sizeof(InjData), offsetof(INJDATA, wcOutput));

    if (fReturn == FALSE)
    {
        // Failed to retrieve class information!
        if (pProc)
            *pProc = NULL;
        if (pClass)
            ZeroMemory(pClass, sizeof(WNDCLASSEX));
        if (pszText)
            pszText[0] = 0;
        return FALSE;
    }
    else
    {
        if (pClass)
        {
            *pClass = InjData.wcOutput;
            // As these pointers come from another process, zero them out to avoid accidental misuse
            pClass->lpszClassName = pClass->lpszMenuName = NULL;
        }

        if (pProc)
            *pProc = InjData.wndproc;

        if (pszText)
            StringCchCopy(pszText, nTextLen, InjData.szText);
        return TRUE;
    }
}
