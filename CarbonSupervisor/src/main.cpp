#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <codecvt>

#include <cstdio>
#include <thread>
#include <iostream>

#include <spdlog/spdlog.h>
#include <MinHook.h>

const uintptr_t ContraptionOffset = 0x1267538;
class Contraption {
private:
	/* 0x0000 */ char pad_0x0000[0x17C];
public:
	/* 0x017C */ int state;

public:
	static Contraption* GetInstance() {
		// TODO sig scan for this (90 48 89 05 ? ? ? ? + 0x4 @ Contraption)
		return *reinterpret_cast<Contraption**>((uintptr_t)GetModuleHandle(nullptr) + ContraptionOffset);
	}
};

/*
 * DLL main thread
 * 
 * @param lpParam The parameter, in this case the module handle
 */
DWORD WINAPI DllMainThread(LPVOID lpParam) {
	AllocConsole();
	freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);

	std::cout << "\n\n\n\n\nHello from CarbonSupervisor\n\n\n\n\n" << std::endl;

	auto hModule = static_cast<HMODULE>(lpParam);

	// Get contraption instance
	auto contraption = Contraption::GetInstance();
	while (contraption == nullptr || contraption->state < 1 || contraption->state > 3) {
		contraption = Contraption::GetInstance();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	// Check for the state changing off of 1 (not loading anymore)
	while (contraption->state == 1) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	spdlog::info("Contraption state: {}", contraption->state);
	spdlog::info("Connecting to pipe");

	auto pipe = CreateFile(
		"\\\\.\\pipe\\CarbonPipe",
		GENERIC_WRITE,
		0,
		nullptr,
		OPEN_EXISTING,
		0,
		nullptr
	);

	if (pipe == INVALID_HANDLE_VALUE) {
		spdlog::error("Failed to connect to pipe");
		return 0;
	}

	spdlog::info("Connected to pipe");

	// Send the loaded packet
	const char* packet = "LOADED-:-";
	DWORD bytesWritten = 0;
	WriteFile(pipe, packet, strlen(packet), &bytesWritten, nullptr);

	spdlog::info("Sent loaded packet");

	// Main loop
	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		static int lastState = 0;
		if (lastState != contraption->state) {
			lastState = contraption->state;
			// Send the state change packet
			std::string packet = "STATECHANGE-:-" + std::to_string(contraption->state);
			WriteFile(pipe, packet.c_str(), packet.size(), &bytesWritten, nullptr);
			spdlog::info("Sent state change packet: {}", contraption->state);
		}
	}

	FreeLibraryAndExitThread(hModule, 0);
	return 0;
}

/*
 * DLL entry point
 * 
 * @param hModule The module handle
 * @param ulReason The reason for calling this function
 * @param lpReserved Reserved
 * @return The exit code of the application
 */
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ulReason, LPVOID lpReserved) {
	if (ulReason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hModule);
		CreateThread(nullptr, 0, DllMainThread, hModule, 0, nullptr);
	}

	return TRUE;
}