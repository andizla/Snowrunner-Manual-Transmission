#pragma once
#include <cstdint>

// Shared state export. Publishes the current vehicle's transmission state in a
// small named shared memory block so other mods in the game process (e.g. a
// gauge cluster) can follow SMT's gear, range and clutch without hooking it.
//
// The block is named "Local\SnowRunnerMT.State.<process id>" and holds one
// SmtSharedState. It is a mapping rather than an exported variable so a reader
// is never left with a pointer into an unloaded DLL: when SMT detaches it
// clears the vehicle flag, and a block whose tick stops advancing is stale.
//
// Readers: open the mapping read-only, check magic, version and size, then copy
// the block while sequence is even and unchanged across the copy (SMT makes it
// odd while it writes). A newer version only appends fields; a reader takes
// what it knows from size.

#define SMT_STATE_NAME_PREFIX L"Local\\SnowRunnerMT.State."
#define SMT_STATE_MAGIC 0x53544D53u // "SMTS"
#define SMT_STATE_VERSION 1u

enum SmtStateFlags : uint32_t {
	SMT_STATE_VEHICLE = 1u << 0,          // a vehicle is driven; the fields below describe it
	SMT_STATE_AUTO_RANGE = 1u << 1,       // SMT's A range (numbered gears at the normal power), not L-, L or L+
	SMT_STATE_CLUTCH = 1u << 2,           // the clutch binding is held
	SMT_STATE_REQUIRE_CLUTCH = 1u << 3,   // [OPTIONS] REQUIRE CLUTCH: a shift without the clutch stalls the engine
	SMT_STATE_REQUIRE_GEAR_HELD = 1u << 4, // [OPTIONS] REQUIRE GEAR HELD: neutral when no gear binding is held
	SMT_STATE_IMMERSIVE = 1u << 5,        // [OPTIONS] IMMERSIVE MODE
	SMT_STATE_GAME_SHIFTING_OFF = 1u << 6, // [OPTIONS] DISABLE GAME SHIFTING
	SMT_STATE_CLUTCH_AXIS = 1u << 7,      // the clutch is bound to a single controller axis: clutch is its travel
	SMT_STATE_FOCUSED = 1u << 8,          // the game window is in front, so the bindings are being read
};

struct SmtSharedState {
	uint32_t magic;           // SMT_STATE_MAGIC
	uint32_t version;         // SMT_STATE_VERSION
	uint32_t size;            // sizeof(SmtSharedState) as written
	volatile long sequence;   // odd while SMT writes the block
	uint64_t tick;            // GetTickCount64() at the last write, about every 16 ms
	uint64_t vehicle;         // the current Vehicle* as a number, 0 with none
	int32_t gear;             // TruckAction->Gear_1: -1 reverse, 0 neutral, 1 up; GetMaxGear() + 1 is the high gear
	int32_t maxGear;          // GetMaxGear()
	int32_t range;            // the range selector: -1 low, 0 none, 1 high
	uint32_t flags;           // SmtStateFlags
	float clutch;             // 0 released to 1 pressed: the axis's travel, or 0 or 1 for a button
	float powerCoef;          // the power coefficient SMT last chose for the low gears
	uint32_t stalls;          // engine stalls SMT has caused for a shift without the clutch
	uint32_t shifts;          // changes of the gear SMT has seen since it loaded
};
static_assert(sizeof(SmtSharedState) == 64, "SmtSharedState is part of the export's version 1 layout");

// Creates the block. Called once from the main thread after the config is loaded.
extern void InitStateExport();
// Publishes the current state. Called from the input thread every tick.
extern void PublishState();
// Counts a stall SMT caused (REQUIRE CLUTCH). Called from the input thread.
extern void CountStateStall();
// Clears the vehicle flag and releases the block.
extern void ShutdownStateExport();
