#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <iostream>
#include <thread>
#include <chrono>
#include <string>

typedef unsigned int uint4_t;

struct Contraption {
	/* 0x000 */ char pad_056[0x17C];
	/* 0x17C */ uint4_t gameStateType;
	/* 0x180 */ char pad_004[0x20];
	/* 0x1A0 */ HWND hWnd;
};

template <typename T>
T FetchClass(uintptr_t address) {
	return *reinterpret_cast<T*>(address);
}

/*
 * Thread entry point for the DLL.
 * @param lpParameter The parameter passed to the thread.
 */
DWORD WINAPI ThreadProc(LPVOID lpParameter) {
	HMODULE hModule = static_cast<HMODULE>(lpParameter);

	// *Create* a named pipe for other processes to connect to
	HANDLE hPipe = CreateNamedPipeA("\\\\.\\pipe\\CarbonSupervisor", PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 1024, 1024, 0, nullptr);
	if (hPipe == INVALID_HANDLE_VALUE) {
		MessageBoxA(nullptr, "Failed to open named pipe", "CarbonSupervisor", MB_OK);
		return 1;
	}

	uintptr_t contraptionAddr = (uintptr_t)GetModuleHandle(nullptr) + 0x12674B8;
	Contraption* contraption = FetchClass<Contraption*>(contraptionAddr);

	while (contraption == nullptr)
		contraption = FetchClass<Contraption*>(contraptionAddr);

	while (contraption->gameStateType < 1 || contraption->gameStateType > 3 || contraption == nullptr || contraption->hWnd == nullptr) {
		contraption = FetchClass<Contraption*>(contraptionAddr);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	while (contraption->gameStateType == 1) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	std::this_thread::sleep_for(std::chrono::seconds(1));

	// Open the supervisor pipe and send `loaded`
	char buffer[] = "loaded";
	DWORD bytesWritten;

	if (!WriteFile(hPipe, buffer, sizeof(buffer), &bytesWritten, nullptr)) {
		MessageBoxA(nullptr, "Failed to write to named pipe", "CarbonSupervisor", MB_OK);
		return 1;
	}

	for (;;) {
		static uint4_t lastGameStateType = contraption->gameStateType;

		if (contraption->gameStateType != lastGameStateType) {
			lastGameStateType = contraption->gameStateType;

			std::cout << "\n\n\n\n\nChanged game state type: " << contraption->gameStateType << "\n\n\n\n\n";

			std::cout << "Game state type: " << contraption->gameStateType << std::endl;

			// Send the game state type to the supervisor
			memset(buffer, 0, sizeof(buffer));
			//buffer[0] = contraption->gameStateType;
			//_itoa(contraption->gameStateType, buffer, 10);
			_itoa_s(contraption->gameStateType, buffer, 10);
			// buffer -> "1" -> 0x31

			if (!WriteFile(hPipe, buffer, sizeof(buffer), &bytesWritten, nullptr)) {
				MessageBoxA(nullptr, "Failed to write to named pipe", "CarbonSupervisor", MB_OK);
				return 1;
			}

			if (contraption->gameStateType == 3) {
				break;
			}

			std::this_thread::sleep_for(std::chrono::seconds(1));
		}

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	// TODO: Send log messages

	FreeLibraryAndExitThread(hModule, 0);
	return 0;
}


/*
 * DLL Entry Point.
 * 
 * @param hModule The handle to the DLL module.
 * @param dwReason The reason for the DLL entry point being called.
 * @param lpReserved Reserved.
 * @return True if the DLL was successfully loaded, false otherwise.
 */
BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
		CreateThread(nullptr, 0, ThreadProc, hModule, 0, nullptr);

		break;
	case DLL_PROCESS_DETACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}
