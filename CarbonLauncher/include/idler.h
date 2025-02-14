#pragma once

#include "pch.h"

namespace CL {
	class Idler {
	public:
		Idler() = default;
		~Idler() = default;

		void IdleBySleeping() const;
		bool* GetEnableIdling() { return &this->enableIdling; }

	private:
		float idleFPSLimit = 9.0f;
		bool enableIdling = true;

		Idler(const Idler&) = delete;
		Idler& operator=(const Idler&) = delete;
	};
} // namespace CL
