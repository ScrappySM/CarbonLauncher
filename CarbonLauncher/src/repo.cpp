#include "managers/repo.h"

namespace Managers {
	Repo::Repo() {
		ZoneScoped;

		this->FetchRepo("https://api.github.com/repos/CarbonMod/Carbon/releases/latest");
	}

	void Repo::FetchRepo(const std::string_view& url) {
		ZoneScoped;

		// Firstly, download the repos JSON file

	}
}
