///////////////////////////////////////////////////////////////////////
//
// Reshade IL2 VREM addon. VR Enhancer Mod for IL2 using reshade
// "hot" reload of mod possible using a Reshade addon as launcher (loaded with the game)
// and a dll containing the mod logic itselve. Mod settings are in uniforms of a technique
// 
// ----------------------------------------------------------------------------------------
// Reshade addon launcher 
// ----------------------------------------------------------------------------------------
// 
// (c) Lefuneste.
//
// All rights reserved.
// https://github.com/xxx
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met :
//
//  * Redistributions of source code must retain the above copyright notice, this
//	  list of conditions and the following disclaimer.
//
//  * Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and / or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// This software is using part of code or algorithms provided by
// * Crosire https://github.com/crosire/reshade  
// * FransBouma https://github.com/FransBouma/ShaderToggler
// * ShortFuse https://github.com/clshortfuse/renodx
// 
/////////////////////////////////////////////////////////////////////////

#include <imgui.h>
#include "loader_addon_shared.h"
#include "loader_on_event.hpp"

//local variables
#if USE_HOT_RELOAD
VREMHotReloader* g_reloader = nullptr;
static bool g_reload_in_progress = false;
static bool g_reload_requested = false;
#endif

SharedState g_shared_state_l;

//****************************************************************
// addon infos
extern "C" __declspec(dllexport) const char* NAME = "IL2 GB VREM";
extern "C" __declspec(dllexport) const char* DESCRIPTION = 
#if USE_HOT_RELOAD
"VR Enhancer Mod for IL2 Great Battle (DEBUG - Hot Reload Enabled).";
#else
"VR Enhancer Mod for IL2 Great Battle (RELEASE - Direct Call).";
#endif

//****************************************************************
// GUI for reshade addon
static void draw_settings(reshade::api::effect_runtime*)
{
#if USE_HOT_RELOAD
    // reload button
    // deactivate button during reloard
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
    ImGui::Separator();

    // debug flag
    ImGui::Checkbox("Debug messages", &g_shared_state_l.debug);

    //capture a fame 
    if (ImGui::Button("Capture frame"))
    {
        g_shared_state_l.button_capture = true;
    }
    else
    {
        g_shared_state_l.button_capture = false;
    }
#else
    // Mode Release : indiquer que le hot reload est désactivé
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "RELEASE MODE - Hot reload & debug messages disabled");
    ImGui::Separator();
#endif
}

//****************************************************************
// to trace overalay is opened
static bool reshade_open_overlay(reshade::api::effect_runtime* runtime, bool open, reshade::api::input_source source)
{

    //trace reshade GUI is active => refresh mod settings with uniforms of VREM_settings

    g_shared_state_l.overlay_is_open = open;

    return false;
}

//*******************************************************************************
// handle button press plus call vrem "on_present" function

#if !USE_HOT_RELOAD
extern "C" __declspec(dllimport) void vrem_on_reshade_present(effect_runtime*);
#endif

static void on_reshade_present(effect_runtime* runtime) {
#if USE_HOT_RELOAD
    if (g_reloader) {
        // Vérifier demande GUI
        if (g_reload_requested) {
            g_reload_requested = false;
            g_reloader->manual_reload();
            g_reload_in_progress = false;  // Réinitialiser après le rechargement
        }
        
        if (g_reloader->get_functions().on_reshade_present) {
            typedef void (*Func)(effect_runtime*);
            ((Func)g_reloader->get_functions().on_reshade_present)(runtime);
        }
        
    }
#else
    vrem_on_reshade_present(runtime);
#endif
}

//*******************************************************************************
// declaration of addon 

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID) 
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
    {
        if (!reshade::register_addon(hModule)) {
            return FALSE;
        }

#if USE_HOT_RELOAD
        // Initialize addon
        WCHAR buf[MAX_PATH];
        const std::filesystem::path dllPath = GetModuleFileNameW(hModule, buf, ARRAYSIZE(buf)) ?
            buf : std::filesystem::path();
        const std::filesystem::path basePath = dllPath.parent_path();
        const std::filesystem::path addonPath = basePath / VREM_ADDON_NAME;
        // intialization is doing mapping between exported VREM function and the function used in "register_event"
        // if fuction are not exported in addon there will be no call
        g_reloader = new VREMHotReloader(addonPath.string());
        reshade::log::message(reshade::log::level::info, "VREM Loader: DEBUG MODE - Hot reload enabled");
#else
        // Mode Release : pas de reloader
        reshade::log::message(reshade::log::level::info, "VREM Loader: RELEASE MODE - Direct calls");
#endif
        // register event : local mapping
        reshade::register_overlay(nullptr, draw_settings);
        reshade::register_event<reshade::addon_event::reshade_present>(on_reshade_present);
        reshade::register_event<reshade::addon_event::reshade_open_overlay>(reshade_open_overlay);

        // register event : call of addon exported functions (see loader_on_event.hpp)
        // reshade::register_event<reshade::addon_event::init_swapchain>(on_init_swapchain);
        reshade::register_event<reshade::addon_event::init_pipeline>(on_init_pipeline);
        // !!! warning !!! to optimize performance there is a filtering setup in on_bind_pipeline() to limit processing to ALLOWED_STAGES
        reshade::register_event<reshade::addon_event::bind_pipeline>(on_bind_pipeline);
        reshade::register_event<reshade::addon_event::init_pipeline_layout>(on_init_pipeline_layout);
        reshade::register_event<reshade::addon_event::draw>(on_draw);
        reshade::register_event<reshade::addon_event::draw_indexed>(on_draw_indexed);
        reshade::register_event<reshade::addon_event::draw_or_dispatch_indirect>(on_draw_indirect);
        reshade::register_event<reshade::addon_event::push_descriptors>(on_push_descriptors);
        reshade::register_event<reshade::addon_event::create_pipeline>(on_create_pipeline);
        // // reshade::register_event<reshade::addon_event::init_pipeline>(on_after_create_pipeline);
        // // reshade::register_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(on_bind_render_targets);
        // reshade::register_event<reshade::addon_event::reshade_present>(on_reshade_present);
        // reshade::register_event<reshade::addon_event::reshade_overlay>(on_reshade_overlay);
        // // reshade::register_event<reshade::addon_event::reshade_reloaded_effects>(on_reshade_reloaded_effects);
        //reshade::register_event<reshade::addon_event::reshade_set_technique_state>(on_reshade_set_technique_state);
        reshade::register_event<reshade::addon_event::destroy_pipeline>(on_destroy_pipeline);
        //reshade::register_event<reshade::addon_event::reshade_reloaded_effects>(on_reshade_reloaded_effects);
      
        break;
    }
    case DLL_PROCESS_DETACH:
    {
               
        // unregister event : local mapping
        reshade::unregister_overlay(nullptr, draw_settings);
        reshade::unregister_event<reshade::addon_event::reshade_present>(on_reshade_present);
        reshade::unregister_event<reshade::addon_event::reshade_open_overlay>(reshade_open_overlay);
        
        // unregister event : call of addon exported functions (see loader_on_event.hpp)
        // reshade::unregister_event<reshade::addon_event::init_swapchain>(on_init_swapchain);
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
        // reshade::unregister_event<reshade::addon_event::reshade_present>(on_reshade_present);
        // reshade::unregister_event<reshade::addon_event::reshade_overlay>(on_reshade_overlay);
        // reshade::unregister_event<reshade::addon_event::reshade_reloaded_effects>(on_reshade_reloaded_effects);
        //reshade::unregister_event<reshade::addon_event::reshade_set_technique_state>(on_reshade_set_technique_state);
        reshade::unregister_event<reshade::addon_event::destroy_pipeline>(on_destroy_pipeline);

#if USE_HOT_RELOAD
        if (g_reloader) {
            delete g_reloader;
            g_reloader = nullptr;
        }
#endif
        // clean remanents objects 
        delete_persistant_objects();

        reshade::unregister_addon(hModule);
        break;
    }
    }
    return TRUE;
}