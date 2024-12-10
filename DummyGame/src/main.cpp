#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shellapi.h>
#include <TlHelp32.h>

#include <MinHook.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <iostream>
#include <thread>
#include <vector>

typedef unsigned int uint4_t;

// LoadLibraryA function pointer
typedef HMODULE(WINAPI* LoadLibraryA_t)(LPCSTR lpLibFileName);

struct Contraption_t {
	/* 0x000 */ char pad_056[0x17C]; // this is genuinely random in the game
	/* 0x17C */ uint4_t gameStateType = 0; // This is random at first but managed
	/* 0x180 */ char pad_004[0x20]; // this is genuinely random in the game
	/* 0x1A0 */ HWND hWnd; // this is random in the game at first but managed
};

#ifdef NDEBUG
static char pad[0x12674B8 - 0x1DC30] = { 0 }; // offset it to replicate the game's memory layout
#else
static_assert(false, "This is only for the release build");
#endif

static Contraption_t* Contraption = new Contraption_t();

int main() {
	spdlog::set_level(spdlog::level::trace);

	// Stop the compiler optimizing the pad array
	if (pad[0] == 1) {
		spdlog::critical("This should never be printed");
	}

	// Get a pointer to the pointer and print it so we can adjust it to be the same as the game
	Contraption_t** ContraptionPtr = &Contraption;

	spdlog::info("Contraption: {}", reinterpret_cast<void*>((uintptr_t)ContraptionPtr - (uintptr_t)GetModuleHandle(nullptr)));

	// Wait 5s
	std::this_thread::sleep_for(std::chrono::seconds(5));

	HINSTANCE hInst = GetModuleHandle(nullptr);
	HWND newHwnd = CreateWindowExA(0, "STATIC", "Dummy Game", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, NULL, NULL, hInst, NULL);

	// Save hwnd to the Contraption struct and set the game state type to 1 (loading screen)
	Contraption->hWnd = (HWND)newHwnd;
	Contraption->gameStateType = 1;

	// Wait 2s
	std::this_thread::sleep_for(std::chrono::seconds(2));

	// Set the game state type to 2 (in-game)
	Contraption->gameStateType = 2;

	// Hook LoadLibraryA so we can detect and block DLL injection (we need to block it
	// because since we aren't the real game it will crash, this is only for testing so seeing
	// the mods injecting isn't important)
	MH_Initialize();
	MH_CreateHook(&LoadLibraryA, (LPVOID)(LoadLibraryA_t)[](LPCSTR lpLibFileName) -> HMODULE {
		std::string libFileName = lpLibFileName;
		spdlog::trace("Blocked DLL injection: {}", libFileName);
		return NULL;
		}, nullptr);

	MH_EnableHook(&LoadLibraryA);

	while (!(GetAsyncKeyState(VK_HOME) & 1 && GetAsyncKeyState(VK_END)) & 1) {
		// Fx sets contraption game state type to x
		if (GetAsyncKeyState(VK_F1) & 1) {
			Contraption->gameStateType = 1;
			spdlog::trace("Game state type is now 1");
		}

		if (GetAsyncKeyState(VK_F2) & 1) {
			Contraption->gameStateType = 2;
			spdlog::trace("Game state type is now 2");
		}

		if (GetAsyncKeyState(VK_F3) & 1) {
			Contraption->gameStateType = 3;
			spdlog::trace("Game state type is now 3");
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	MH_DisableHook(&LoadLibraryA);
	MH_Uninitialize();

	return 0;
}
