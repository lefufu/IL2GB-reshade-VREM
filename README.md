# VREM: VR Enhancer (ReShade Addon)
## Description
VREM is a specialized ReShade addon designed to enhance the VR experience in IL-2 Sturmovik: Great Battles. While optimized for Virtual Reality, most features are also compatible with 2D gameplay.

    [!CAUTION]
    Multiplayer Compatibility: This mod may be prohibited on some multiplayer servers, as the ReShade library itself can be flagged by anti-cheat systems. Use at your own discretion in online environments.

🚀 Key Features
🎯 Sight & Aiming

    Customizable Sight: Adjust the intensity and opacity of the gun sight via the ReShade GUI.

    Stereo Options: Toggle the sight display for the left eye, right eye, or both.

🛠️ Visual Enhancements

    Airframe Masking: Prevents sun halos and light rays from bleeding through the cockpit airframe.

    Night Map Mode: Darkens the in-game map to prevent dazzle during night sorties.

        Hotkey: CTRL + U to toggle.

    Floating Pilot Notes: Displays reference notes (checklists, maps) directly in front of the player.

        Hotkey: K to toggle / SHIFT + K to cycle through textures.

        Position and scale are fully adjustable in the GUI.

🏷️ Labels & UI

    Smart Label Masking: Airfield and mission target labels are now correctly hidden by the airframe.

    Immersive Icons: Force a uniform gray color on all labels to reduce the "cheating" look of bright UI elements.

    UI Cleanup: Option to remove the directional triangles at the edge of the screen.

⏱️ Integrated Stopwatch

A dedicated navigation stopwatch that is easy to read in both VR and 2D.

    Visibility: J to toggle.

    Controls: SHIFT + K (Start/Pause) | CTRL + K (Reset).

⚙️ Advanced Rendering & Shaders
* Optimized Rendering Chain

Unlike standard ReShade effects that apply to the final image, VREM2 allows you to select specific techniques to be rendered before the UI (Map, HUD, etc.) is drawn. This ensures a clean interface and better compatibility with OpenComposite.
* Dedicated VREM Shaders

VREM2 includes modified versions of industry-standard shaders, optimized to affect only specific parts of the scene:
* Selective Rendering: Technicolor2, Color Matrix, Fake HDR, and LumaSharpen can be restricted to "Cockpit only" or "External only".
* Smart Deband: selectively apply debanding to the sky, the sea, or both.
* SmartFXAA: A custom FXAA (tuned with Claude AI) that avoids blurring cockpit textures or losing distant aircraft against the horizon. Includes a Debug Mode for visual tuning.

🙏 Acknowledgments

This software is using part of code or algorithms provided by
Crosire https://github.com/crosire/reshade  
FransBouma https://github.com/FransBouma/ShaderToggler
ShortFuse https://github.com/clshortfuse/renodx

I would not have been able to deliver it without their help !


## Installation

[!IMPORTANT] Neither ReShade nor VREM will modify or overwrite any original files of your IL-2 Sturmovik: Great Battles installation.
1. ReShade Setup
Download: Get ReShade from reshade.me.
⚠️ Crucial: You must download the version with Add-on support.
Launch: Run the ReShade_Setup_*_Addon.exe.
Select Game: Point the installer to IL-2.exe (found in your IL-2/bin/game/ folder).
Rendering API: Select DirectX 10/11/12.
❌ Do not select OpenXR.
Effects: You can untick SweetFX by CeeJay.dk unless you specifically need it.
Add-ons: Do not select any additional add-ons during this stage.
2. VREM Setup
Download: Get the latest version from the VREM Releases.
Install Options:
Option A (Recommended): Use OvGME. Simply place the zip package into your OvGME MOD folder and enable it.
Option B (Manual):
Unzip reshade_VREM_IL2GB.zip.
Copy the bin folder from the archive into your main IL-2 installation folder.
This will correctly place the following files:
bin\game\reshade-shaders\Shaders\VREM\
bin\game\VREM_shaderreplace\
bin\game\VREM_launcher.addon64

## 🗑️ Uninstallation

### Removing ReShade

Run the ReShade_Setup_*_Addon.exe again, select IL-2.exe, and choose the Uninstall option.

### Removing VREM
* **OvGME:** Simply disable the mod in the application.
* **Manual:** Delete the following items from your `IL-2/bin/game/` directory:
    * The folder `reshade-shaders\Shaders\VREM\`
    * The folder `VREM_shaderreplace\`
    * The file `VREM_launcher.addon64`
