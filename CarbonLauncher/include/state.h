#pragma once

#include "pch.h"

#include "idler.h"

#include "managers/game.h"

enum class Tab {
	Discover,
	Console,
	Settings,
};

class State {
public:
	// GUI State
	Tab currentTab = Tab::Discover;
	Idler idler;

	// Managers
	Managers::Game gameManager;

public:
	static State* GetInstance() {
		static State instance;
		return &instance;
	}

private:
	State() = default;
	~State() = default;

	State(const State&) = delete;
	State& operator=(const State&) = delete;
};
