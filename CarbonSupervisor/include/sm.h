#pragma once

#define WIN32_LEAN_AND_MEAN

#include <string>
#include <Windows.h>

#include <spdlog/spdlog.h>

enum ContraptionState {
	LOADING = 1,
	IN_GAME = 2,
	MAIN_MENU = 3
};

namespace SM {
	const uintptr_t ContraptionOffset = 0x1267538;

	struct LogMessage {
		int colour;
		std::string message;
	};

	class Console {
	private:
		virtual ~Console() {}

	public:
		virtual void Log(const std::string&, WORD colour, WORD LogType) = 0;
		virtual void LogNoRepeat(const std::string&, WORD colour, WORD LogType) = 0;

		void Hook();
	};

	class Contraption {
	private:
		/* 0x0000 */ char pad_0x0000[0x58];
	public:
		/* 0x0058 */ Console* console;
	private:
		/* 0x0060 */ char pad_0x0060[0x11C];
	public:
		/* 0x017C */ int state;

	public:
		static Contraption* GetInstance() {
			// TODO sig scan for this (90 48 89 05 ? ? ? ? + 0x4 @ Contraption)
			auto contraption = *reinterpret_cast<Contraption**>((uintptr_t)GetModuleHandle(nullptr) + ContraptionOffset);
			while (contraption == nullptr || contraption->state < LOADING || contraption->state > 3) {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				contraption = SM::Contraption::GetInstance();

				static int i = 0;
				if (i++ % 30 == 0) { // Every 3 seconds
					spdlog::warn("Waiting for Contraption...");
				}
			}

			return contraption;
		}

		void WaitForStateEgress(ContraptionState state) const {
			while (this->state == (int)state) {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}

		void OnStateChange(std::function<void(ContraptionState)> callback) const {
			while (true) {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				static int lastState = 0;
				if (lastState != this->state) {
					lastState = this->state;
					callback((ContraptionState)lastState);
				}
			}
		}
	};
}
