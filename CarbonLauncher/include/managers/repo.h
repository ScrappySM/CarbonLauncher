#pragma once

#include "pch.h"

class Mod {
public:
	Mod() = default;
	~Mod() = default;

	struct GHAttachment {
		std::string author;
		std::string repo;
		std::vector<std::string> verifiedHash; // TODO
	} ghAttachment;
};

namespace Managers {
	class Repo {
	public:
		Repo() = default;
		~Repo() = default;

		Repo(const Repo&) = delete;
		Repo& operator=(const Repo&) = delete;

		void FetchRepo(const std::string_view& url);

	private:
		std::vector<Mod> mods;
	};
} // namespace Managers
