#include "shared.h"
#include "state_export.h"
#include "config.h"
#include "input.h"
#include "game_data.h"
#include "memory.h"
#include "gui.h"
#include <cstddef>

extern std::unordered_map <Vehicle*, std::atomic<bool>> IsInAuto;
extern std::atomic<float> currentCoef;

static HANDLE stateMapping = nullptr;
static SmtSharedState* stateBlock = nullptr;
static std::atomic<uint32_t> stateStalls = 0;
static uint32_t stateShifts = 0;
static uint64_t lastVehicle = 0;
static int32_t lastGear = 0;

// Copies everything after the sequence into the block, with the sequence odd
// while it does.
static void WriteBlock(const SmtSharedState& s) {
	if (!stateBlock) {
		return;
	}
	const size_t from = offsetof(SmtSharedState, tick);
	InterlockedIncrement(&stateBlock->sequence);
	MemoryBarrier();
	memcpy(reinterpret_cast<char*>(stateBlock) + from, reinterpret_cast<const char*>(&s) + from, sizeof(SmtSharedState) - from);
	MemoryBarrier();
	InterlockedIncrement(&stateBlock->sequence);
}

void InitStateExport() {
	wchar_t name[96];
	swprintf_s(name, L"%s%lu", SMT_STATE_NAME_PREFIX, GetCurrentProcessId());
	stateMapping = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(SmtSharedState), name);
	if (!stateMapping) {
		LogMessage("State export: CreateFileMapping failed", GetLastError());
		return;
	}
	stateBlock = static_cast<SmtSharedState*>(MapViewOfFile(stateMapping, FILE_MAP_WRITE, 0, 0, sizeof(SmtSharedState)));
	if (!stateBlock) {
		LogMessage("State export: MapViewOfFile failed", GetLastError());
		CloseHandle(stateMapping);
		stateMapping = nullptr;
		return;
	}
	stateBlock->magic = SMT_STATE_MAGIC;
	stateBlock->version = SMT_STATE_VERSION;
	stateBlock->size = sizeof(SmtSharedState);
	LogMessage("State export: publishing the transmission state");
}

void CountStateStall() {
	stateStalls++;
}

void PublishState() {
	if (!stateBlock) {
		return;
	}
	SmtSharedState s = {};
	s.tick = GetTickCount64();
	s.range = range.load();
	s.powerCoef = currentCoef.load();
	s.stalls = stateStalls.load();
	if (iniConfig["OPTIONS"]["REQUIRE CLUTCH"].as<bool>()) s.flags |= SMT_STATE_REQUIRE_CLUTCH;
	if (iniConfig["OPTIONS"]["REQUIRE GEAR HELD"].as<bool>()) s.flags |= SMT_STATE_REQUIRE_GEAR_HELD;
	if (iniConfig["OPTIONS"]["IMMERSIVE MODE"].as<bool>()) s.flags |= SMT_STATE_IMMERSIVE;
	if (iniConfig["OPTIONS"]["DISABLE GAME SHIFTING"].as<bool>()) s.flags |= SMT_STATE_GAME_SHIFTING_OFF;
	if (GetForegroundWindow() == window) s.flags |= SMT_STATE_FOCUSED;
	bool held = false;
	float travel = 0.0f;
	const bool axis = ClutchState(held, travel);
	if (held) s.flags |= SMT_STATE_CLUTCH;
	if (axis) s.flags |= SMT_STATE_CLUTCH_AXIS;
	s.clutch = axis ? travel : held ? 1.0f : 0.0f;
	if (auto veh = GetCurrentVehicle()) {
		s.flags |= SMT_STATE_VEHICLE;
		if (IsInAuto[veh].load()) s.flags |= SMT_STATE_AUTO_RANGE;
		s.vehicle = reinterpret_cast<uint64_t>(veh);
		s.gear = veh->TruckAction->Gear_1;
		s.maxGear = veh->GetMaxGear();
		if (s.vehicle == lastVehicle && s.gear != lastGear) {
			stateShifts++;
		}
		lastVehicle = s.vehicle;
		lastGear = s.gear;
	}
	else {
		lastVehicle = 0;
	}
	s.shifts = stateShifts;
	WriteBlock(s);
}

void ShutdownStateExport() {
	if (stateBlock) {
		SmtSharedState s = {};
		s.tick = GetTickCount64();
		WriteBlock(s);
		UnmapViewOfFile(stateBlock);
		stateBlock = nullptr;
	}
	if (stateMapping) {
		CloseHandle(stateMapping);
		stateMapping = nullptr;
	}
}
