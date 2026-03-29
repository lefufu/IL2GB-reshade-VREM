VREM: VR Enhancer (ReShade Addon)

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
Optimized Rendering Chain

Unlike standard ReShade effects that apply to the final image, VREM2 allows you to select specific techniques to be rendered before the UI (Map, HUD, etc.) is drawn. This ensures a clean interface and better compatibility with OpenComposite.
Dedicated VREM Shaders

VREM2 includes modified versions of industry-standard shaders, optimized to affect only specific parts of the scene:

    Selective Rendering: Technicolor2, Color Matrix, Fake HDR, and LumaSharpen can be restricted to "Cockpit only" or "External only".

    Smart Deband: selectively apply debanding to the sky, the sea, or both.

    SmartFXAA: A custom FXAA (tuned with Claude AI) that avoids blurring cockpit textures or losing distant aircraft against the horizon. Includes a Debug Mode for visual tuning.

🙏 Acknowledgments

This software is using part of code or algorithms provided by
Crosire https://github.com/crosire/reshade  
FransBouma https://github.com/FransBouma/ShaderToggler
ShortFuse https://github.com/clshortfuse/renodx

I would not have been able to deliver it without their help !
