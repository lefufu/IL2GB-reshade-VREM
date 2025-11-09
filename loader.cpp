///////////////////////////////////////////////////////////////////////
//
// DCS VREM Hot Reload Loader - VERSION FINALE
// Fichier: DCS_VREM_loader.cpp
//
///////////////////////////////////////////////////////////////////////

// #define IMGUI_DISABLE_INCLUDE_IMCONFIG_H
// #define ImTextureID unsigned long long

// #include <vector>
// #include <filesystem>
// #include <unordered_map>
// #include <windows.h>
// #include <chrono>
// #include <thread>
// #include <reshade.hpp>

#include <imgui.h>
#include "loader_addon_shared.h"
#include "loader_on_event.hpp"


//global variables
VREMHotReloader* g_reloader = nullptr;

//local variables
static bool g_reload_in_progress = false;
static bool g_reload_requested = false;
static SharedState g_shared_state;


extern "C" __declspec(dllexport) const char* NAME = "IL2 GB VREM";
extern "C" __declspec(dllexport) const char* DESCRIPTION = "VR Enhancer Mod for IL2 Great Battle.";

//****************************************************************
// GUI for reshade addon
static void draw_settings(reshade::api::effect_runtime*)
{
    
    reshade::log::message(reshade::log::level::info,
        "draw_settings() called");
    
    // Désactiver le bouton pendant le rechargement
    if (g_reload_in_progress) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Reload dll"))
    {
        g_reload_requested = true;
        g_reload_in_progress = true;
    }

    if (g_reload_in_progress) {
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Reloading...");
    }

}

static void on_register_overlay(reshade::api::effect_runtime*)
{
    reshade::log::message(reshade::log::level::info,
        "***** on_register_overlay : g_shared_state.overlay_is_open = true");
}

static bool reshade_open_overlay(reshade::api::effect_runtime* runtime, bool open, reshade::api::input_source source)
{

    //trace reshade GUI is active => refresh mod settings with uniforms of VREM_settings
    std::stringstream s;
    s << "***** reshade_open_overlay : g_shared_state.overlay_is_open, open =" << open << "; ";
    reshade::log::message(reshade::log::level::info, s.str().c_str());

    g_shared_state.overlay_is_open = open;

    return false;
}

//*******************************************************************************
// handle button press plus call vrem on present function

static void on_reshade_present(effect_runtime* runtime) {

    if (g_reloader) {
        // Vérifier demande GUI
        if (g_reload_requested) {
            g_reload_requested = false;
            g_reloader->manual_reload();
            g_reload_in_progress = false;  // Réinitialiser après le rechargement
        }

        // Vérifier Ctrl+F5
        if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_F5) & 0x8000)) {
            static bool key_pressed = false;
            if (!key_pressed) {
                g_reloader->manual_reload();
                key_pressed = true;
            }
        }
        else {
            static bool key_pressed = false;
            key_pressed = false;
        }

        if (g_reloader->get_functions().on_reshade_present) {
            typedef void (*Func)(effect_runtime*);
            ((Func)g_reloader->get_functions().on_reshade_present)(runtime);
        }
    }
}

/*
static void on_reshade_overlay(effect_runtime* runtime) {
   
    reshade::log::message(reshade::log::level::info,
        "***** on_reshade_overlay : g_shared_state.overlay_is_open = true");
}
*/

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID) 
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
    {
        if (!reshade::register_addon(hModule)) {
            return FALSE;
        }

        WCHAR buf[MAX_PATH];
        const std::filesystem::path dllPath = GetModuleFileNameW(hModule, buf, ARRAYSIZE(buf)) ?
            buf : std::filesystem::path();
        const std::filesystem::path basePath = dllPath.parent_path();
        const std::filesystem::path addonPath = basePath / "IL2GB_VREM.dll";

        g_reloader = new VREMHotReloader(addonPath.string());

        // reshade::register_event<reshade::addon_event::init_swapchain>(on_init_swapchain);
        // reshade::register_event<reshade::addon_event::init_pipeline>(on_init_pipeline);
        /*
        reshade::register_event<reshade::addon_event::bind_pipeline>(on_bind_pipeline);
        reshade::register_event<reshade::addon_event::init_pipeline_layout>(on_init_pipeline_layout);
        reshade::register_event<reshade::addon_event::draw>(on_draw);
        reshade::register_event<reshade::addon_event::draw_indexed>(on_draw_indexed);
        reshade::register_event<reshade::addon_event::draw_or_dispatch_indirect>(on_draw_indirect);
        reshade::register_event<reshade::addon_event::push_descriptors>(on_push_descriptors);
        reshade::register_event<reshade::addon_event::create_pipeline>(on_create_pipeline);
        reshade::register_event<reshade::addon_event::init_pipeline>(on_after_create_pipeline);
        reshade::register_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(on_bind_render_targets);
        reshade::register_event<reshade::addon_event::reshade_present>(on_reshade_present);
        reshade::register_event<reshade::addon_event::reshade_overlay>(on_reshade_overlay);
        reshade::register_event<reshade::addon_event::reshade_reloaded_effects>(on_reshade_reloaded_effects);
        // reshade::register_event<reshade::addon_event::reshade_set_technique_state>(on_reshade_set_technique_state);
        reshade::register_event<reshade::addon_event::destroy_pipeline>(on_destroy_pipeline);
        */

        reshade::register_event<reshade::addon_event::reshade_present>(on_reshade_present);
        reshade::register_overlay(nullptr, draw_settings);
        reshade::register_event<reshade::addon_event::reshade_open_overlay>(reshade_open_overlay);
        //reshade::register_event<reshade::addon_event::reshade_reloaded_effects>(on_reshade_reloaded_effects);
      
        break;
    }
    case DLL_PROCESS_DETACH:
    {
        
        reshade::unregister_event<reshade::addon_event::reshade_present>(on_reshade_present);
        //reshade::unregister_event<reshade::addon_event::reshade_overlay>(on_reshade_overlay);
        /* reshade::unregister_event<reshade::addon_event::init_swapchain>(on_init_swapchain);
        reshade::unregister_event<reshade::addon_event::init_pipeline>(on_init_pipeline);
        reshade::unregister_event<reshade::addon_event::bind_pipeline>(on_bind_pipeline);
        reshade::unregister_event<reshade::addon_event::init_pipeline_layout>(on_init_pipeline_layout);
        reshade::unregister_event<reshade::addon_event::draw>(on_draw);
        reshade::unregister_event<reshade::addon_event::draw_indexed>(on_draw_indexed);
        reshade::unregister_event<reshade::addon_event::draw_or_dispatch_indirect>(on_draw_indirect);
        reshade::unregister_event<reshade::addon_event::push_descriptors>(on_push_descriptors);
        reshade::unregister_event<reshade::addon_event::create_pipeline>(on_create_pipeline);
        reshade::unregister_event<reshade::addon_event::init_pipeline>(on_after_create_pipeline);
        reshade::unregister_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(on_bind_render_targets);
        reshade::unregister_event<reshade::addon_event::reshade_present>(on_reshade_present);
        reshade::unregister_event<reshade::addon_event::reshade_overlay>(on_reshade_overlay);
        reshade::unregister_event<reshade::addon_event::reshade_reloaded_effects>(on_reshade_reloaded_effects);
        // reshade::unregister_event<reshade::addon_event::reshade_set_technique_state>(on_reshade_set_technique_state);
        reshade::unregister_event<reshade::addon_event::destroy_pipeline>(on_destroy_pipeline);
        */

        if (g_reloader) {
            delete g_reloader;
            g_reloader = nullptr;
        }

        reshade::unregister_addon(hModule);
        break;
    }
    }
    return TRUE;
}