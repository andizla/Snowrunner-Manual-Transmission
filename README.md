# Snowrunner Manual Transmission

This mod disables SnowRunner's automatic shitbox transmission. \
Shifting can either be done with the game's ui shifter and keybinds to shift up/down in the auto position (IMMERSIVE MODE) or completely with keybinds (DISABLE GAME SHIFTING).

## Installation

1. Get the latest release [here.](https://github.com/drafty46/SMT/releases/latest)
2. Extract files from archive into game folder, next to SnowRunner.exe.
3. The mod will load on game launch.

#### Use [this subscribe mod](https://mod.io/g/snowrunner/m/manual-text-for-shifter#description) to change the shifter text from AUTO {gear} to GEAR {gear}
![alt text](image-1.png)

## Usage

The mod's menu, where all configuration is done, is bound to the delete key by default.

![alt text](image.png)

To change a keybind left click on it's blue box, press the desired key/keys and then right click on the box to confirm. \
**NOTE: For axis that return to one end like pedals you need to confirm before releasing them.**    \
To clear an already set keybind right click on it's box. \
Changes take effect immediately but aren't saved unless you use the save button.

**Ranges** - use this if you don't have a physical range toggle. There's LOW, NORMAL and HIGH.  \
Cycling up/down is done with 'RANGE LOW'/'RANGE HIGH' keybinds. (**Must be a single button**)   \
When selected, LOW/HIGH simulate their key being pressed, so bind the same button for the range + the actual button to press.

**AWD / diff lock / handbrake** - bindable like any other action (see the Keyboard/Controller
tables). Each press toggles the corresponding state on/off. Current state is always shown as an
on-screen indicator (stacked above the RANGE indicator, green when active) whenever you're in a
vehicle - this works even when toggled remotely, since it doesn't depend on a keybind being set.
Turn it off with the **SHOW STATUS OVERLAY** option if you don't want it on screen.

## Remote control (PowerShell / scripting)

The mod can be driven externally over a local named pipe, so any script or tool can trigger the
same actions as a keybind (shift gears, toggle range, open the menu, etc.) and read back the
current transmission state.

1. **ENABLE REMOTE CONTROL** is on by default (in the mod's OPTIONS panel, or `[OPTIONS]` in
   `SMT.ini`). Turn it off if you don't want the pipe created.
2. From PowerShell:
   ```powershell
   Import-Module .\PowerShell\SnowRunnerMT.psm1
   Test-SMTConnection      # $true if the game is running and reachable
   Get-SMTStatus           # current gear, max gear, auto mode, range, AWD, diff lock, handbrake
   Set-SMTGear 3           # shift directly to gear 3
   Invoke-SMTGearUp        # shift up one gear
   Invoke-SMTRangeHigh     # cycle range up
   Invoke-SMTToggleAWD     # toggle all-wheel drive
   Invoke-SMTToggleDiffLock   # toggle the diff lock
   Invoke-SMTToggleHandbrake  # toggle the handbrake
   Show-SMTMenu            # toggle the mod's in-game menu
   Get-SMTActionList       # every action name the mod accepts
   ```

### Making the module available in every PowerShell window

`Import-Module .\PowerShell\SnowRunnerMT.psm1` only works while your current directory has that
relative path, and only for the current session. To have the `SMT-*` commands available in any new
PowerShell window (Windows PowerShell 5.1 or PowerShell 7+) without an explicit `Import-Module`,
symlink the script into your user module path so PowerShell's module auto-loading picks it up by
name:

```powershell
$src = "<path to repo>\PowerShell\SnowRunnerMT.psm1"
$targets = @(
    "$HOME\Documents\PowerShell\Modules\SnowRunnerMT"        # PowerShell 7+
    "$HOME\Documents\WindowsPowerShell\Modules\SnowRunnerMT" # Windows PowerShell 5.1
)
foreach ($dir in $targets) {
    New-Item -ItemType Directory -Path $dir -Force | Out-Null
    New-Item -ItemType SymbolicLink -Path (Join-Path $dir "SnowRunnerMT.psm1") -Target $src -Force | Out-Null
}
```

Because it's a symlink rather than a copy, future edits to the script in the repo take effect
immediately — no reinstall step needed. Creating a symbolic link requires either an elevated
PowerShell session or Developer Mode enabled (Settings > Privacy & security > For developers); if
that's not possible, copy the file into those folders instead of symlinking it.

Once installed, any new PowerShell window can skip the `Import-Module` line and call the commands
above directly.

Under the hood this is a line-based text protocol on `\\.\pipe\SnowRunnerMT`: write a command
line (e.g. `GEAR 3`, `GEAR UP`, `CLUTCH`, `RANGE HIGH`, `AWD`, `DIFF LOCK`, `HANDBRAKE`,
`SHOW MENU`, `STATUS`, `LIST`, `PING`) and read back one response line. Any language that can open
a Windows named pipe can talk to it, not just PowerShell. Commands are queued and executed on the
mod's existing input thread, so they behave exactly like a bound key press.

## Shared state for other mods

Other mods in the game process can follow the transmission without the pipe. Every input tick
(about 60 times a second) the mod publishes a 64 byte block in the named shared memory
`Local\SnowRunnerMT.State.<process id>`: the current vehicle, its gear and top gear, the range,
the clutch (held, and its travel from 0 to 1 when it is bound to a single controller axis), the
power coefficient of the low gears, the stalls caused by a shift without the clutch, the options
that change the shifting, and the time of the write. The layout and how to read it are in
`DLL/state_export.h`: open the mapping read-only, check the magic, version and size, and copy the
block while its sequence number is even and unchanged. When the mod unloads it clears the
vehicle flag, and a block whose time stops advancing is stale.

## Credits

### [Ferrster](https://github.com/Ferrster) for [Snowrunner-Manual-Gearbox-Mod](https://github.com/Ferrster/Snowrunner-Manual-Gearbox-Mod) - reverse engineered game functions and data structures
### [ThirteenAG](https://github.com/ThirteenAG) for [Ultimate-ASI-Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) - injector
### [microsoft](https://github.com/microsoft) for [Detours](https://github.com/microsoft/Detours) - library
### [ocornut](https://github.com/ocornut) for [DearImgui](https://github.com/ocornut/imgui) - library
### [Rookfighter](https://github.com/Rookfighter) for [inifile-cpp](https://github.com/Rookfighter/inifile-cpp) - library
### [Rebzzel](https://github.com/Rebzzel) for [kiero](https://github.com/Rebzzel/kiero) - library
### [rdbo](https://github.com/rdbo) for [ImGui-DirectX-11-Kiero-Hook](https://github.com/rdbo/ImGui-DirectX-11-Kiero-Hook) - Imgui+kiero integration
### [wgois](https://github.com/wgois) for [OIS](https://github.com/wgois/OIS) - library
### [Tessil](https://github.com/Tessil) for [ordered-map](https://github.com/Tessil/ordered-map) - library
### [nothings](https://github.com/nothings) for [stb](https://github.com/nothings/stb) - library