#pragma once

#include "pch.h"

class Idler {
public:
	Idler() = default;
	~Idler() = default;

	Idler(const Idler&) = delete;
	Idler& operator=(const Idler&) = delete;

	void IdleBySleeping() const;

	bool* GetEnableIdling() { return &this->enableIdling; }

private:
	float idleFPSLimit = 9.0f;

#ifdef NDEBUG
	bool enableIdling = true;
#else
	bool enableIdling = false;
#endif
};
