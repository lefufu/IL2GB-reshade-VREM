///////////////////////////////////////////////////////////////////////
//
// Reshade IL2 VREM addon. VR Enhancer Mod for IL2 using reshade
// "hot" reload of mod possible using a Reshade addon as launcher (loaded with the game)
// and a dll containing the mod logic itselve. Mod settings are in uniforms of a technique
// 
// ----------------------------------------------------------------------------------------
//  initialization & cleanup of VREM mod
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
#include <filesystem>

#include "loader_addon_shared.h"
#include "VREM_settings.h"
#include "addon_logs.h"
#include "addon_objects.h"

//*******************************************************************************
// variables shared at addon level
SharedState* g_shared_state = nullptr;
bool addon_init = false;

// definition of action triggered by shaders/pipeline
std::unordered_map<uint32_t, Shader_Definition> shader_by_hash =
{
	// ** fix for rotor **
	{ 0xC0CC8D69, Shader_Definition(action_replace, Feature::Rotor, L"AH64_rotorPS.cso", 0) },
	{ 0x349A1054, Shader_Definition(action_replace, Feature::Rotor, L"AH64_rotor2PS.cso", 0) },
	{ 0xD3E172D4, Shader_Definition(action_replace, Feature::Rotor, L"UH1_rotorPS.cso", 0) },
	// ** fix for IHADSS **
	{ 0x2D713734, Shader_Definition(action_replace, Feature::IHADSS, L"IHADSS_PNVS_PS.cso", 0) },
	{ 0xDF141A84, Shader_Definition(action_replace, Feature::IHADSS, L"IHADSS_PS.cso", 0) },
	{ 0x45E221A9, Shader_Definition(action_replace, Feature::IHADSS, L"IHADSS_VS.cso", 0) },
	// ** label masking and color/sharpen/deband **
	// to start spying texture for depthStencil (Vs associated with global illumination PS)
	// and inject modified CB CperFrame
	{ 0x4DDC4917, Shader_Definition(action_log | action_injectCB, Feature::GetStencil, L"", 0) },
	{ 0x57D037A0, Shader_Definition(action_injectCB, Feature::Sky, L"", 0) },
	// { 0x4DDC4917, Shader_Definition(action_log , Feature::GetStencil, L"", 0) },
	// global PS for all changes
	//{ 0xBAF1E52F, Shader_Definition(action_replace | action_injectText | action_log, Feature::Global, L"global_PS_2.cso", 0) },
	{ 0xBAF1E52F, Shader_Definition(action_replace | action_injectText, Feature::Global, L"global_PS_2.cso", 0) },
	//TODO VS associated with global PS 2 for draw increase : 8DB626CD
	{ 0x8DB626CD, Shader_Definition(action_log , Feature::VS_global2, L"", 0) },
	// Label PS 
	{ 0x6CEA1C47, Shader_Definition(action_replace | action_injectText, Feature::Label , L"labels_PS.cso", 0) },
	// ** NS430 **
	// to start spying texture for screen texture and disable frame (Vs associated with NS430 screen PS below)
	{ 0x52C97365, Shader_Definition(action_replace, Feature::NS430, L"VR_GUI_MFD_VS.cso", 0) },
	// to start spying texture for screen texture (Vs associated with NS430 screen EDF9F8DD for su25T&UH1, not same res. texture !)
	{ 0x8439C716, Shader_Definition(action_log, Feature::NS430, L"", 0) },
	// inject texture in global GUI and filter screen display (same shader for both)
	{ 0x99D562, Shader_Definition(action_replace | action_injectText, Feature::NS430 , L"VR_GUI_MFD_PS.cso", 0) },
	// disable NS430 frame, shared with some cockpit parts (can not be done by skip)
	{ 0xEFD973A1, Shader_Definition(action_replace, Feature::NS430 , L"NS430__framePS.cso", 0) },
	// disable NS430 screen background (done in shader because shared with other objects than NS430)
	{ 0x6EF95548, Shader_Definition(action_replace, Feature::NS430, L"NS430_screen_back.cso", 0) },
	// to filter out call for GUI and MFD
	{ 0x55288581, Shader_Definition(action_log, Feature::GUI, L"", 0) },
	//  ** identify game config **
	// to define if VR is active or not (2D mirror view of VR )
	{ 0x886E31F2, Shader_Definition(action_identify | action_log, Feature::VRMode, L"", 0) },
	// VS drawing cockpit parts to define if view is in welcome screen or map
	{ 0xA337E177, Shader_Definition(action_identify, Feature::mapMode, L"", 0) },
	//  ** reflection on instrument, done by GCOCKPITIBL of CperFrame **
	// A10C PS 
	{ 0xC9F547A7, Shader_Definition(action_injectCB , Feature::NoReflect , L"", 0) },
	// AH64 + F4 PS 
	{ 0x7BB48FB, Shader_Definition(action_injectCB , Feature::NoReflect , L"", 0) },

	//  ** NVG **
	{ 0xE65FAB66, Shader_Definition(action_replace , Feature::NVG , L"NVG_extPS.cso", 0) },
	//  ** identify render target ** (VS associated with first global PS)
	{ 0x936B2B6A, Shader_Definition(action_log , Feature::Effects , L"", 0) },

};

//
// structure to contain shaders/pipeline to process, regarding mod option selected
std::unordered_map<uint64_t, Shader_Definition> filtered_pipeline;



extern "C" {
    //*******************************************************************************
    __declspec(dllexport) void vrem_init(
        reshade::api::device* device,
        reshade::api::command_queue* queue,
        reshade::api::swapchain* swapchain,
        PersistentPipelineData* persistent_data,
        SharedState* shared_state
    )
    {
        // set the g_shared_state vaiable of the addon to the variable shared by the launcher
        g_shared_state = shared_state;

        log_addon_init();
        // reshade::log::message(reshade::log::level::info, "** DCS VREM: Initialisation de l'addon...");

        // Code du DLL_PROCESS_ATTACH
        WCHAR buf[MAX_PATH];
        const std::filesystem::path dllPath = GetModuleFileNameW(nullptr, buf, ARRAYSIZE(buf)) ?
            buf : std::filesystem::path();
        const std::filesystem::path basePath = dllPath.parent_path();
    
        
        // initialize addon variables 

        g_shared_state->s_last_time_point = std::chrono::high_resolution_clock::now();

        addon_init = true;

        //TODO : init mod params ?

		//display the shader list
		// display_shader_by_hash();


        // reshade::log::message(reshade::log::level::info,"DCS VREM: register done...");
    }

    //*******************************************************************************
    __declspec(dllexport) void vrem_cleanup(PersistentPipelineData* persistent_data)
    {
        // reshade::log::message(reshade::log::level::info, "DCS VREM: Nettoyage de l'addon...");
        log_addon_cleanup();

        // Nettoyer uniquement les données temporaires
        // shader_code.clear();
        // s_data_to_delete.clear();

        // NE PAS FAIRE :
        // - pipeline_by_handle.clear()
        // - pipeline_by_hash.clear()
        // - reshade::unregister_overlay()
        // - reshade::unregister_event()
        // - device->destroy_pipeline(substitute_pipeline)
    }
}
