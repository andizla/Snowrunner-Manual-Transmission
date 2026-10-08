#pragma once
#include "shared.h"
#include <functional>
#include <unordered_map>

namespace SMT {
	class KeyListener : public OIS::KeyListener {
	public:
		bool keyPressed(const OIS::KeyEvent& e) override;
		bool keyReleased(const OIS::KeyEvent& e) override;
	};
	class JoyStickListener : public OIS::JoyStickListener {
	public:
		bool buttonPressed(const OIS::JoyStickEvent& e, int button) override;

		bool buttonReleased(const OIS::JoyStickEvent& e, int button) override;

		bool axisMoved(const OIS::JoyStickEvent& e, int axis) override;

		bool sliderMoved(const OIS::JoyStickEvent& e, int sliderID) override;

		bool povMoved(const OIS::JoyStickEvent& e, int pov) override;
	};
	class MouseListener : public OIS::MouseListener {
	public:
		bool mousePressed(const OIS::MouseEvent& e, OIS::MouseButtonID button) override;
		bool mouseReleased(const OIS::MouseEvent& e, OIS::MouseButtonID button) override;
		bool mouseMoved(const OIS::MouseEvent& e) override;
	};
}

extern void InitInput();
extern void ShutdownInput();
extern OIS::InputManager* inputManager;
extern OIS::Keyboard* keyboard;
extern OIS::Mouse* mouse;
extern std::vector<OIS::JoyStick*> joystickList;
extern std::set<std::string> tempPressed;
extern std::atomic<bool> keepAliveInput;
extern std::atomic<int32_t> range;
extern std::unordered_map<std::string, std::function<void()>> bindFunctions;

// Snapshot of the current vehicle's transmission state, refreshed every
// input tick regardless of window focus. Lets other threads (e.g. the IPC
// server) report status without touching game memory themselves.
extern std::atomic<bool> remoteVehiclePresent;
extern std::atomic<int32_t> remoteGearSnapshot;
extern std::atomic<int32_t> remoteMaxGearSnapshot;
extern std::atomic<bool> remoteAutoSnapshot;
extern std::atomic<bool> remoteAWDSnapshot;
extern std::atomic<bool> remoteDiffLockSnapshot;
extern std::atomic<bool> remoteHandbrakeSnapshot;

// The clutch: held is whether its binding is held; when it is bound to a single
// controller axis, travel is that axis's travel, 0 released to 1 pressed, and
// the result is true. Called from the input thread.
extern bool ClutchState(bool& held, float& travel);