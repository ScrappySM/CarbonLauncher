#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <iostream>

/*
 * Thread entry point for the DLL.
 * @param lpParameter The parameter passed to the thread.
 */
DWORD WINAPI ThreadProc(LPVOID lpParameter) {
	HMODULE hModule = static_cast<HMODULE>(lpParameter);

	// TODO: Handle logic such as opening a pipe.

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
