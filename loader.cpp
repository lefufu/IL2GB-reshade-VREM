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
// #if USE_HOT_RELOAD
VREMHotReloader* g_reloader = nullptr;
static bool g_reload_in_progress = false;
static bool g_reload_requested = false;
// #endif

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

#ifndef _DEBUG
extern void vrem_on_reshade_present(reshade::api::effect_runtime* runtime);
extern void vrem_init(reshade::api::device* device,reshade::api::command_queue* queue, reshade::api::swapchain* swapchain,PersistentPipelineData* persistent_data,SharedState* shared_state);
extern void vrem_cleanup(PersistentPipelineData* persistent_data);
#endif

/*
//****************************************************************
// to launch addon initialization if no hot reloader
#ifndef _DEBUG
static void OnInitSwapchain(reshade::api::swapchain* swapchain)
{
    
    reshade::log::message(reshade::log::level::info, "VREM launcher : OnInitSwapchain");
    if (!g_shared_state_l.init_done)
    {
        g_shared_state_l.init_done = true;
        auto* device = swapchain->get_device();
        reshade::log::message(reshade::log::level::info, "VREM launcher : OnInitSwapchain call vrem_init");
        vrem_init(device, nullptr, swapchain, nullptr, &g_shared_state_l);
    }
}
#endif
*/

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

    // shader hunter mode : display full framelogs and filter 1 PS
    ImGui::Checkbox("Shader hunter", &g_shared_state_l.shader_hunter);

	//shader hunter mode
    ImGui::RadioButton("Disable", &g_shared_state_l.color_PS, 0); ImGui::SameLine();
    ImGui::RadioButton("Color", &g_shared_state_l.color_PS, 1);

    //capture a fame 
    if (ImGui::Button("Capture frame"))
    {
        g_shared_state_l.button_capture = true;
    }
    else
    {
        g_shared_state_l.button_capture = false;
    }


    ImGui::Text("Number of pipeline: %d", g_shared_state_l.PSshader_list.size() - 1);
    ImGui::Text("current hunted pipeline index");
    if (ImGui::Button("-"))
    {
        g_shared_state_l.PSshader_index--;
        if (g_shared_state_l.PSshader_index < 0)  g_shared_state_l.PSshader_index = g_shared_state_l.PSshader_list.size() - 1;
    }
    ImGui::SameLine();
    ImGui::InputInt("", &g_shared_state_l.PSshader_index, 0, static_cast<int>(g_shared_state_l.PSshader_list.size()) - 1);
    ImGui::SameLine();
    if (ImGui::Button("+"))
    {
        g_shared_state_l.PSshader_index++;
        if (g_shared_state_l.PSshader_index > g_shared_state_l.PSshader_list.size() - 1) g_shared_state_l.PSshader_index = 0;
    }

    // display info on pipeline
    if (g_shared_state_l.PSshader_list.size() > 0)
    {
        uint64_t handle = g_shared_state_l.PSshader_list[g_shared_state_l.PSshader_index];
        ImGui::Text("Hunted pipeline handle: 0x%llx", handle);
        uint32_t hash = 0;
		save_pipeline* p_found = nullptr;
        if (handle != 0)
        {

            for (auto& p : g_shared_state_l.VREM_pipelines.saved_pipelines) {
                if (p.pipeline.handle == handle) {
                    hash = p.hash[0];
                    p_found = &p;
                }
            }
        }
        ImGui::SameLine();
        ImGui::Text("    first hash = 0x%llx", hash);
        //ImGui::Text("    first hash = 0x%llx", p_found->hash[0]);
        static char buffer[256] = "marking...";
        ImGui::InputText("Comment", buffer, sizeof(buffer));

        if (ImGui::Button("write shader in log"))
        {
            log_shader_marked(p_found, buffer);
        }
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

        // Initialize addon
        WCHAR buf[MAX_PATH];
        const std::filesystem::path dllPath = GetModuleFileNameW(hModule, buf, ARRAYSIZE(buf)) ?
            buf : std::filesystem::path();
        const std::filesystem::path basePath = dllPath.parent_path();
        const std::filesystem::path addonPath = basePath / VREM_ADDON_NAME;
        // intialization is doing mapping between exported VREM function and the function used in "register_event"
        // if fuction are not exported in addon there will be no call
//#ifdef _DEBUG
        g_reloader = new VREMHotReloader(addonPath.string());
//#endif
#ifdef _DEBUG
        reshade::log::message(reshade::log::level::info, "VREM Loader: DEBUG MODE - Hot reload enabled");
#else
        // Mode Release : pas de reloader
        reshade::log::message(reshade::log::level::info, "VREM Loader: RELEASE MODE - Direct calls");
#endif
/*
#ifndef _DEBUG
        //intialization of addon if no hot reload
        reshade::register_event<reshade::addon_event::init_swapchain>(OnInitSwapchain);
#endif   
*/
#ifndef _DEBUG
        vrem_init(nullptr, nullptr, nullptr, nullptr, &g_shared_state_l);
#endif 
        // register event : local mapping
        reshade::register_overlay(nullptr, draw_settings);
        reshade::register_event<reshade::addon_event::reshade_present>(on_reshade_present);
        reshade::register_event<reshade::addon_event::reshade_open_overlay>(reshade_open_overlay);
        reshade::register_event<reshade::addon_event::init_pipeline>(on_init_pipeline);
        // !!! warning !!! to optimize performance there is a filtering setup in on_bind_pipeline() to limit processing to ALLOWED_STAGES
        reshade::register_event<reshade::addon_event::bind_pipeline>(on_bind_pipeline);
        reshade::register_event<reshade::addon_event::init_pipeline_layout>(on_init_pipeline_layout);
        reshade::register_event<reshade::addon_event::draw>(on_draw);
        reshade::register_event<reshade::addon_event::draw_indexed>(on_draw_indexed);
        reshade::register_event<reshade::addon_event::draw_or_dispatch_indirect>(on_draw_indirect);
        reshade::register_event<reshade::addon_event::push_descriptors>(on_push_descriptors);
        reshade::register_event<reshade::addon_event::create_pipeline>(on_create_pipeline);
        reshade::register_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(on_bind_render_targets);
        reshade::register_event<reshade::addon_event::reshade_overlay>(on_reshade_overlay); 
        reshade::register_event<reshade::addon_event::reshade_reloaded_effects>(on_reshade_reloaded_effects); 

        //reshade::register_event<reshade::addon_event::destroy_pipeline>(on_destroy_pipeline); 
        // register event : call of addon exported functions (see loader_on_event.hpp)
        // reshade::register_event<reshade::addon_event::init_swapchain>(on_init_swapchain);
        // // reshade::register_event<reshade::addon_event::reshade_present>(on_reshade_present);
        // // reshade::register_event<reshade::addon_event::init_pipeline>(on_after_create_pipeline);
        //reshade::register_event<reshade::addon_event::reshade_set_technique_state>(on_reshade_set_technique_state);
 
        break;
        return TRUE;
    }
    case DLL_PROCESS_DETACH:
    {
#ifndef _DEBUG   
//#ifdef _DEBUG  
        //cleaning of addon variables if no hot reload
        vrem_cleanup(nullptr);
#endif        
        // unregister event : local mapping
        reshade::unregister_overlay(nullptr, draw_settings);
        reshade::unregister_event<reshade::addon_event::reshade_present>(on_reshade_present);
        reshade::unregister_event<reshade::addon_event::reshade_open_overlay>(reshade_open_overlay);
        reshade::unregister_event<reshade::addon_event::init_pipeline>(on_init_pipeline);
        reshade::unregister_event<reshade::addon_event::bind_pipeline>(on_bind_pipeline);
        reshade::unregister_event<reshade::addon_event::init_pipeline_layout>(on_init_pipeline_layout);
        reshade::unregister_event<reshade::addon_event::draw>(on_draw);
        reshade::unregister_event<reshade::addon_event::draw_indexed>(on_draw_indexed);
        reshade::unregister_event<reshade::addon_event::draw_or_dispatch_indirect>(on_draw_indirect);
        reshade::unregister_event<reshade::addon_event::push_descriptors>(on_push_descriptors);
        reshade::unregister_event<reshade::addon_event::create_pipeline>(on_create_pipeline);
        reshade::unregister_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(on_bind_render_targets);
        reshade::unregister_event<reshade::addon_event::reshade_overlay>(on_reshade_overlay);
        reshade::unregister_event<reshade::addon_event::reshade_reloaded_effects>(on_reshade_reloaded_effects);

/* #ifndef _DEBUG
        //intialization of addon if no hot reload
        reshade::unregister_event<reshade::addon_event::init_swapchain>(OnInitSwapchain);
#endif */   

        //reshade::unregister_event<reshade::addon_event::destroy_pipeline>(on_destroy_pipeline);
        // unregister event : call of addon exported functions (see loader_on_event.hpp)
        // reshade::unregister_event<reshade::addon_event::init_swapchain>(on_init_swapchain);
        // reshade::unregister_event<reshade::addon_event::reshade_present>(on_reshade_present);
        //reshade::unregister_event<reshade::addon_event::reshade_set_technique_state>(on_reshade_set_technique_state);
        // reshade::unregister_event<reshade::addon_event::init_pipeline>(on_after_create_pipeline);


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