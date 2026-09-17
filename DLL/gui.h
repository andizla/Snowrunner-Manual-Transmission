#pragma once
#include "shared.h"

#ifdef SMT_RESHADE_ADDON
// addon_gui.cpp: the menu and the HUD rows are drawn from ReShade's overlay callbacks, there is no Present hook
extern void RegisterOverlay();
extern std::atomic<bool> smtReady;   // set once hooks, input and pipe are up; the overlay draws nothing before that
#else
typedef HRESULT(__stdcall* Present) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef uintptr_t PTR;

extern Present originalPresent;
extern HRESULT __stdcall hookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
#endif
extern std::atomic<bool> showGui;
extern void InitGui();
extern void ShutdownGui();
extern HWND window;
extern std::atomic<bool> isGuiInitialized;