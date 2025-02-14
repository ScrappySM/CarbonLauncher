#pragma once

#include "pch.h"

#include "tab.h"

namespace CL {
	class State {
	public:
		static State& GetInstance() {
			static State instance;
			return instance;
		}

		Tab currentTab = Tab::Discover;

	private:
		State() = default;
		~State() = default;

		State(const State&) = delete;
		State& operator=(const State&) = delete;
	};
} // namespace CL
