#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <iostream>
#include <thread>
#include <chrono>
#include <string>

typedef unsigned int uint4_t;

struct Contraption {
	/* 0x000 */ HWND hWnd;
	/* 0x008 */ char pad_008[0x174];
	/* 0x17C */ uint4_t gameStateType;
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

	std::this_thread::sleep_for(std::chrono::seconds(2));

	auto contraptionAddr = (uintptr_t)GetModuleHandle(nullptr) + 0x12674B8;
	auto contraption = FetchClass<Contraption*>(contraptionAddr);
	while (contraption == nullptr) {
		contraption = FetchClass<Contraption*>(contraptionAddr);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	std::this_thread::sleep_for(std::chrono::seconds(1));
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
