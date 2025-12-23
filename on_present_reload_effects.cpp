///////////////////////////////////////////////////////////////////////
//
// Reshade IL2 VREM addon. VR Enhancer Mod for IL2 using reshade
// "hot" reload of mod possible using a Reshade addon as launcher (loaded with the game)
// and a dll containing the mod logic itselve. Mod settings are in uniforms of a technique
// 
// ----------------------------------------------------------------------------------------
//  VREM mod "on present" and "on_reload" call => read settings from uniform
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

#include <reshade.hpp>
#include <thread>

#include "loader_addon_shared.h"
#include "VREM_settings.h"
#include "addon_functions.h"
#include "addon_logs.h"

using namespace reshade::api;

extern "C" {
    //*******************************************************************************
    _declspec(dllexport) void vrem_on_reshade_present(reshade::api::effect_runtime* runtime)
    {

        // not used, 
        reshade::api::device* device = runtime->get_device();

        // to get VREM settings => read from uniform values
        if (g_shared_state->overlay_is_open || addon_init == true) {
            // Récupérer l'état actuel des uniforms
            get_settings_from_uniforms(runtime);
            addon_init = false;

        }

        g_shared_state->device = device;
        /*
        // fps limiter function
        // if (g_shared_state->s_fps_limit <= 0)
        if (VREM_setting[DUMMY] == 0)
            return;

        const auto time_per_frame = std::chrono::high_resolution_clock::duration(std::chrono::seconds(1)) / int(VREM_setting[FPS_LIMIT]);
        const auto next_time_point = g_shared_state->s_last_time_point + time_per_frame;

        while (next_time_point > std::chrono::high_resolution_clock::now())
            std::this_thread::sleep_for(std::chrono::high_resolution_clock::duration(std::chrono::milliseconds(1)));

        g_shared_state->s_last_time_point = next_time_point;
        */

        // generate filtered shader list and clone pipelines if needed
        if (g_shared_state->filtered_pipeline_to_setup) {

            g_shared_state->filtered_pipeline_to_setup = false;
            setup_filtered_pipelines(g_shared_state->device);

        }

        /*
        // frame capture by button on GUI
        
        if (flag_capture)
        {
            flag_capture = false;
            log_end_capture_frame();
        }
        else
        {
            if (g_shared_state->button_capture)
            {
                flag_capture = true;
                log_start_capture_frame();

            }
        } */

        
        // Fin de capture seulement si une frame réelle a commencé
        if (flag_capture && frame_started)
        {
            flag_capture = false;
            frame_started = false;
            log_end_capture_frame();
        }

        // Détection front bouton
        static bool last_button = false;
        bool button = g_shared_state->button_capture;

        if (button && !last_button)
        {
            request_capture = true;
        }
        last_button = button;
        
    }


    //*******************************************************************************
    _declspec(dllexport) void vrem_on_reshade_reloaded_effects(effect_runtime* runtime)
    {

        log_effect_reloaded();

        // reload all uniforms
        get_settings_from_uniforms(runtime);
    }
}