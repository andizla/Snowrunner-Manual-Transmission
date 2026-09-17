// Entry point of the ReShade add-on build (SMT.addon.vcxproj, output SMT.addon64). The ASI build uses dllmain.cpp.
// Same start-up order as there; the differences are the ReShade registration, the host check and the module pin.
#include "shared.h"
#include "gui.h"
#include "config.h"
#include "input.h"
#include "memory.h"
#include "ipc.h"

// ReShade lists the add-on under these.
extern "C" __declspec(dllexport) const char* NAME = "SnowRunner Manual Transmission";
extern "C" __declspec(dllexport) const char* DESCRIPTION = "Manual gearbox for SnowRunner: SMT by drafty46 with EvanTrow's additions, built as a ReShade add-on. The menu is in the ReShade overlay.";

HMODULE g_hModule = NULL;
static bool g_started = false;
static bool g_pinned = false;    // the module stays mapped through ReShade's add-on unload and reload cycles
std::atomic<bool> hasConsole = false;
std::atomic<bool> alive = true;
std::ofstream logFile;

void AttachConsole()
{
	if (!hasConsole) {
		AllocConsole();
		hasConsole = true;
		freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
		freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
		freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
		SetConsoleTitleA("Logging Console");

		std::cout.clear();
		std::cerr.clear();
		std::clog.clear();

		LogMessage("Started Logging");
	}
}

void DetachConsole() {
	if (hasConsole) {
		LogMessage("Console Detached. You can close this window.");
		FreeConsole();
		hasConsole = false;
	}
}

void DetachDLL() {
	CreateThread(nullptr, 0, [](LPVOID lpParam) -> DWORD {
		HMODULE hMod = (HMODULE)lpParam;
		FreeLibraryAndExitThread(hMod, 0);
		return 0;
		}, g_hModule, 0, nullptr);
}

DWORD WINAPI MainThread(LPVOID lpReserved)
{
	logFile.open("SMT_LOG.txt");
	logFile << std::unitbuf;
	LogMessage("ReShade add-on build");
	LogMessage(g_pinned ? "addon: module pinned, survives ReShade's add-on reloads" : "addon: PIN FAILED, a ReShade add-on reload would unload this module");
	LoadIniConfig();
	InitMemory();
	InitGui();
	while (!isGuiInitialized) { Sleep(100); }
	InitInput();
	InitIPC();
	smtReady = true;
	LogMessage("addon: ready");
	return TRUE;
}

// ReShade can load add-ons into any program it runs in. The game functions exist only in SnowRunner.exe.
static bool HostIsTheGame()
{
	wchar_t exe[MAX_PATH] = {};
	GetModuleFileNameW(nullptr, exe, MAX_PATH);
	const wchar_t* base = exe;
	for (const wchar_t* p = exe; *p; p++) if (*p == L'\\' || *p == L'/') base = p + 1;
	return _wcsicmp(base, L"SnowRunner.exe") == 0;
}

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		g_hModule = hMod;
		DisableThreadLibraryCalls(hMod);
		if (!HostIsTheGame())
			return FALSE;
		// The ASI build hooks the same game functions and opens the same pipe, so the two never run together.
		if (GetModuleHandleW(L"SMT.asi") != nullptr)
			return FALSE;
		if (!reshade::register_addon(hMod))
			return FALSE;
		// ReShade unloads and reloads its add-ons whenever the game drops a graphics device, and SnowRunner does that
		// during start-up. This module hooks game functions and runs its own threads for the whole session, so it
		// stays mapped: FreeLibrary then leaves it alone, ReShade keeps the registration as external and finds it
		// again on the next pass. ReShade.log notes each pass with "was not unregistered", which is expected.
		{
			HMODULE pinned = nullptr;
			g_pinned = GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCWSTR>(&DllMain), &pinned) != 0;
		}
		RegisterOverlay();
		g_started = true;
		CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);
		break;
	case DLL_PROCESS_DETACH:
		if (!g_started) break;
		// At process exit the other threads are already gone and the game is tearing down; unwinding hooks and
		// input objects here would only risk a hang.
		if (lpReserved != nullptr) { logFile.close(); break; }
		reshade::unregister_addon(hMod);
		ShutdownIPC();
		ShutdownInput();
		do { Sleep(100); } while (keepAliveInput);
		ShutdownMemory();
		ShutdownGui();
		DetachConsole();
		logFile.close();
		break;
	}
	return TRUE;
}
