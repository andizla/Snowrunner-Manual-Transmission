// The overlay unit of the ReShade add-on build (SMT.addon.vcxproj). The ASI build uses gui.cpp, which hooks Present
// through kiero and brings its own Dear ImGui. Here ReShade owns the device, fonts, input and drawing. The menu is a
// registered overlay window, shown while the ReShade overlay is open. The HUD rows are drawn from the reshade_overlay
// event, which ReShade runs on every frame once an add-on listens to it. A change to the menu in gui.cpp needs the
// same change here.
#include "shared.h"
#include "gui.h"
#include "config.h"
#include "input.h"
#include "memory.h"

HWND window = NULL;
std::atomic<bool> showGui = false;
std::atomic<bool> isGuiInitialized = false;
std::atomic<bool> smtReady = false;

extern std::atomic<bool> alive;

// Overlay state as last reported by ReShade or set from here. The SHOW MENU binding flips showGui on the input
// thread, and the next frame asks ReShade to follow.
static std::atomic<bool> overlayOpen = false;

struct FindCtx { DWORD pid; HWND best; };

static BOOL CALLBACK FindGameWindow(HWND h, LPARAM lp) {
	FindCtx* c = (FindCtx*)lp;
	DWORD pid = 0;
	GetWindowThreadProcessId(h, &pid);
	if (pid != c->pid || !IsWindowVisible(h) || GetWindow(h, GW_OWNER)) return TRUE;
	RECT rc = {};
	GetClientRect(h, &rc);
	if (rc.right - rc.left < 640 || rc.bottom - rc.top < 400) return TRUE;
	c->best = h;
	return FALSE;
}

void InitGui() {
	// The input system binds to the main window, which must exist and stay the same for a few seconds first.
	HWND last = NULL;
	int stable = 0;
	for (int i = 0; i < 1200 && alive; i++) {
		FindCtx c{ GetCurrentProcessId(), NULL };
		EnumWindows(FindGameWindow, (LPARAM)&c);
		if (c.best && c.best == last) {
			if (++stable >= 6) { window = c.best; break; }
		}
		else {
			stable = 0;
		}
		last = c.best;
		Sleep(500);
	}
	LogMessage("addon: game window", (uintptr_t)window);
	isGuiInitialized = true;
}

void ShutdownGui() {
	alive = false;
}

static void DrawHudLabel(int32_t row, const char* text, ImU32 color) {
	const float fontSize = 30.0f;
	ImVec2 display = ImGui::GetIO().DisplaySize;
	ImDrawList* drawList = ImGui::GetForegroundDrawList();
	ImVec2 pos(display.x * 0.88f, display.y * (0.975f - row * 0.045f));
	ImVec2 size = ImGui::CalcTextSize(text) * (fontSize / ImGui::GetFontSize());
	// The ASI build drew a translucent black image behind each row. A rounded rectangle takes its place.
	drawList->AddRectFilled(ImVec2(pos.x - 12.0f, pos.y - 3.0f), ImVec2(pos.x + size.x + 12.0f, pos.y + size.y + 3.0f), IM_COL32(0, 0, 0, 110), 6.0f);
	drawList->AddText(ImGui::GetFont(), fontSize, ImVec2(pos.x + 1.0f, pos.y + 1.0f), IM_COL32(0, 0, 0, 192), text);
	drawList->AddText(ImGui::GetFont(), fontSize, pos, color, text);
}

static bool OnOpenOverlay(reshade::api::effect_runtime*, bool open, reshade::api::input_source) {
	overlayOpen = open;
	showGui = open;
	return false;   // never block the change
}

static void OnOverlayFrame(reshade::api::effect_runtime* runtime) {
	if (!alive || !smtReady) return;

	bool wanted = showGui;
	if (wanted != overlayOpen) {
		if (runtime->open_overlay(wanted, reshade::api::input_source::keyboard))
			overlayOpen = wanted;
		else
			showGui = overlayOpen.load();
	}

	auto veh = GetCurrentVehicle();
	if (!veh) return;

	if (iniConfig["KEYBOARD"]["RANGE HIGH"].as<std::string>() != "NONE" || iniConfig["KEYBOARD"]["RANGE LOW"].as<std::string>() != "NONE" ||
		iniConfig["CONTROLLER"]["RANGE HIGH"].as<std::string>() != "NONE" || iniConfig["CONTROLLER"]["RANGE LOW"].as<std::string>() != "NONE") {
		switch (range.load()) {
		case -1: DrawHudLabel(0, "RANGE: LOW", IM_COL32(255, 255, 255, 255)); break;
		case 0: DrawHudLabel(0, "RANGE: NORMAL", IM_COL32(255, 255, 255, 255)); break;
		case 1: DrawHudLabel(0, "RANGE: HIGH", IM_COL32(255, 255, 255, 255)); break;
		}
	}

	// TruckAction is a pointer read on the render thread, so it is checked on every frame.
	if (iniConfig["OPTIONS"]["SHOW STATUS OVERLAY"].as<bool>() && veh->TruckAction) {
		int32_t row = 1;
		struct StateIndicator { const char* label; bool active; };
		StateIndicator indicators[] = {
			{ "AWD", veh->TruckAction->AWD },
			{ "DIFF LOCK", veh->TruckAction->Diff },
			{ "HANDBRAKE", veh->TruckAction->Handbrake },
		};
		for (auto& ind : indicators) {
			std::string text = std::string(ind.label) + ": " + (ind.active ? "ON" : "OFF");
			DrawHudLabel(row++, text.c_str(), ind.active ? IM_COL32(120, 255, 120, 255) : IM_COL32(255, 255, 255, 255));
		}
	}
}

static void DrawBindingTable(const char* id, const char* section, const char* heading, ImVec2 size, int32_t& count) {
	ImGui::BeginChild(id, size, ImGuiChildFlags_Borders);
	std::string tableId = std::string("##table_") + section;
	if (ImGui::BeginTable(tableId.c_str(), 2, ImGuiTableFlags_ScrollX)) {
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn(heading, ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableHeadersRow();
		for (auto entry : iniConfig[section]) {
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Dummy(ImVec2(0, 0.1f));
			ImGui::TextUnformatted(entry.first.c_str());
			ImGui::TableSetColumnIndex(1);
			std::string buttonText = entry.second.as<std::string>() + "##" + std::to_string(++count);
			if (ImGui::Button(buttonText.c_str())) {
				iniConfig[section][entry.first] = "LISTENING";
				tempPressed.clear();
			}
			if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
				if (entry.second.as<std::string>() == "LISTENING")
					iniConfig[section][entry.first] = "FOUND";
				else
					iniConfig[section][entry.first] = "NONE";
			}
		}
		ImGui::EndTable();
	}
	ImGui::EndChild();
}

static void DrawOptionsTable(ImVec2 size) {
	ImGui::BeginChild("##options", size, ImGuiChildFlags_Borders);
	if (ImGui::BeginTable("##table_options", 2, ImGuiTableFlags_ScrollX)) {
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Options", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableHeadersRow();
		for (auto entry : iniConfig["OPTIONS"]) {
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Dummy(ImVec2(0, 0.1f));
			ImGui::TextUnformatted(entry.first.c_str());
			ImGui::TableSetColumnIndex(1);
			std::string label = (entry.second.as<bool>() ? "True##" : "False##") + entry.first;
			if (ImGui::Button(label.c_str()))
				iniConfig["OPTIONS"][entry.first] = !entry.second.as<bool>();
		}
		ImGui::EndTable();
	}
	ImGui::EndChild();
}

static void DrawMenu(reshade::api::effect_runtime*) {
	ImVec2 display = ImGui::GetIO().DisplaySize;
	ImGui::SetWindowSize(ImVec2(display.x * 0.6f, display.y * 0.55f), ImGuiCond_FirstUseEver);
	if (!smtReady) {
		ImGui::TextUnformatted("Starting: waiting for the game window, hooks and input.");
		return;
	}

	if (remoteVehiclePresent)
		ImGui::Text("Vehicle: gear %d of %d, %s, range %d", (int)remoteGearSnapshot.load(), (int)remoteMaxGearSnapshot.load(), remoteAutoSnapshot ? "automatic" : "manual", (int)range.load());
	else
		ImGui::TextUnformatted("Vehicle: none");

	int32_t count = 0;
	const ImGuiStyle& style = ImGui::GetStyle();
	ImVec2 avail = ImGui::GetContentRegionAvail();
	float footer = ImGui::GetFrameHeightWithSpacing() * 2.0f + ImGui::GetFontSize() * 2.0f;
	float height = avail.y - footer;
	if (height < 240.0f) height = 240.0f;
	float spacing = style.ItemSpacing.x * 2.0f;
	float bindingWidth = (avail.x - spacing) * 0.39f;
	float optionsWidth = avail.x - spacing - bindingWidth * 2.0f;

	DrawBindingTable("##keyboard", "KEYBOARD", "Keyboard", ImVec2(bindingWidth, height), count);
	ImGui::SameLine();
	DrawBindingTable("##controller", "CONTROLLER", "Controller", ImVec2(bindingWidth, height), count);
	ImGui::SameLine();
	DrawOptionsTable(ImVec2(optionsWidth, height));

	if (ImGui::Button("Save config"))
		SaveIniConfig();
	ImGui::SameLine();
	ImGui::TextWrapped("To change a binding, left click it, press the keys, buttons or axis, then right click to confirm. Right click clears a binding.");
}

void RegisterOverlay() {
	reshade::register_overlay("Manual Transmission", &DrawMenu);
	reshade::register_event<reshade::addon_event::reshade_overlay>(&OnOverlayFrame);
	reshade::register_event<reshade::addon_event::reshade_open_overlay>(&OnOpenOverlay);
}
