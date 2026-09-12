///////////////////////////////////////////////////////////////////////
//
// Reshade IL2 VREM addon. VR Enhancer Mod for IL2 using reshade
// "hot" reload of mod possible using a Reshade addon as launcher (loaded with the game)
// and a dll containing the mod logic itselve. Mod settings are in uniforms of a technique
// 
// ----------------------------------------------------------------------------------------
// on_reshade_render_technique : disable technique rendering
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

#include "loader_addon_shared.h"
#include "addon_functions.h"
#include "addon_objects.h"
#include "addon_logs.h"

#include "to_string.hpp"

using namespace reshade::api;



#ifdef _DEBUG
extern "C" {
#endif
	// *******************************************************************************************************
	// vrem_on_reshade_render_technique() : to call when effects are reloaded, should not do aything before real effect compilation (another call from on_present)
	// called a lot !n
	VREM_EXPORT void vrem_on_reshade_render_technique(reshade::api::effect_runtime* runtime, reshade::api::effect_technique technique, reshade::api::command_list* cmd_list, reshade::api::resource_view rtv, reshade::api::resource_view rtv_srgb)
	{

#if _DEBUG_CRASH 
		reshade::log::message(reshade::log::level::info, "addon - vrem_on_reshade_render_technique started");
#endif

		// browse the list of techniques to see if this one is in the list of techniques to skip
		/*
        for (auto& t : g_shared_state->technique_vector)
        {
            if (t.technique == technique)
            {
                return true;
            }
        }
		*/

		//if (a_shared.render_technique && a_shared.technique_compiled)
		{
			reshade::log::message(reshade::log::level::info, "addon - vrem_on_reshade_render_technique : technique rendering skipped");
			//return true; // skip rendering of the technique
		}
		//return false; // reshade will render the technique as usual

#if _DEBUG_CRASH 
		reshade::log::message(reshade::log::level::info, "addon - vrem_on_reshade_render_technique ended");
#endif

	}
#ifdef _DEBUG
}
#endif