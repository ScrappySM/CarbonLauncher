#pragma once

#include "pch.h"

#include "state.h"

/// <summary>
/// Applys a highlighted colour to ImGui elements based on if the current tab is the same as the tab passed in
/// </summary>
/// <param name="tab"></param>
/// <returns>If the colour was applied (so you know if you need to pop the style colours after)</returns>
static bool ApplyHighlight(Tab tab) {
	static State* state = State::GetInstance();

	static ImVec4 colours[3] = {};
	if (colours[0].x == 0) {
		colours[0] = ImGui::GetStyleColorVec4(ImGuiCol_Button);
		colours[1] = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
		colours[2] = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);

		colours[0].w += 0.3f;
		colours[1].w += 0.3f;
		colours[2].w += 0.3f;
	}

	if (state->currentTab == tab) {
		ImGui::PushStyleColor(ImGuiCol_Button, colours[0]);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colours[1]);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, colours[1]);

		return true;
	}

	return false;
}

/// <summary>
/// Renders a tab button
/// </summary>
/// <param name="tab">The tab to switch to when the button is clicked</param>
/// <param name="label">What to display on the button</param>
/// <param name="tooltip">The tooltip to display when hovering over the button</param>
static void RenderTab(Tab tab, const char* label, const char* tooltip) {
	static State* state = State::GetInstance();
	bool highlighted = ApplyHighlight(tab);

	if (ImGui::Button(label, ImVec2(48, 48))) {
		state->currentTab = tab;
	}

	ImGui::SetItemTooltip(tooltip);

	if (highlighted) {
		ImGui::PopStyleColor(3);
	}
}
