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

// *******************************************************************************************************
// Initialize counters 
//
void intialize_counters()
{
    // to know the current display : 2D, VR standard: left/right, VR quad view : left eye outer + inner + right eye outer+inner
    a_shared.count_display = 0;

    a_shared.cb_inject_values.mapMode = 1.0;
    a_shared.depthStencil_copy_started = false;
    a_shared.do_not_draw = false;
    a_shared.cb_inject_values.GUItodraw = 0.0;

    a_shared.render_effect = false;
    a_shared.track_for_render_target = false;

    a_shared.last_feature = Feature::Null;

    // a_shared.CB_copied[CPERFRAME_CB_NB] = false;

    // initialize flags for texture copy
    a_shared.current_DS_handle = 0;
    for (auto& [handle, ds_copy] : a_shared.saved_DS) {
        ds_copy.copied = false;
    }


}


extern "C" {
    //*******************************************************************************
    _declspec(dllexport) void vrem_on_reshade_present(reshade::api::effect_runtime* runtime)
    {

        // not used, 
        reshade::api::device* device = runtime->get_device();
        g_shared_state->device = device;

        // initialize counter to identfiy what to do when in the next frame
        intialize_counters();

		// to get VREM settings => read from uniform values (if overlay is open or at first init)
        if (g_shared_state->overlay_is_open || addon_init == true) {
            get_settings_from_uniforms(runtime);
            addon_init = false;
        }

        // create all pipeline_layouts for pushing dedicated CB (if not created)
        create_all_modified_CB_layout(device);

		// create all pipelines layput for pushing dedicated RV (if not created)
        create_RV_pipeline_layout(device);

        // generate filtered shader list and clone pipelines if needed
        if (g_shared_state->filtered_pipeline_to_setup) {

            g_shared_state->filtered_pipeline_to_setup = setup_filtered_pipelines(g_shared_state->device, runtime);

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
        
		//force capture for testing
        // flag_capture = true;
    }


    //*******************************************************************************
    _declspec(dllexport) void vrem_on_reshade_reloaded_effects(effect_runtime* runtime)
    {

        log_effect_reloaded();

        // reload all uniforms
        get_settings_from_uniforms(runtime);
    }
}