
///////////////////////////////////////////////////////////////////////
//
// Reshade IL2 VREM addon. VR Enhancer Mod for IL2 using reshade
// "hot" reload of mod possible using a Reshade addon as launcher (loaded with the game)
// and a dll containing the mod logic itselve. Mod settings are in uniforms of a technique
// 
// ----------------------------------------------------------------------------------------
//  draw* : reset tracking flags and skip draw if needed
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
#include <unordered_map>


#include "loader_addon_shared.h"
#include "addon_functions.h"
#include "addon_objects.h"
#include "VREM_settings.h"
#include "addon_logs.h"

#include "to_string.hpp"


using namespace reshade::api;

//*******************************************************************************************************
// clear tracking flags to avoid tracking resources if push_descriptor did not detect it...

static void clear_tracking_flags()
{

	// if (a_shared.track_for_depthStencil || a_shared.track_for_NS430 || a_shared.do_not_draw)
	if (track_for_depthStencil || do_not_draw)
	{
		log_reset_tracking();
		track_for_depthStencil = false;
		//a_shared.track_for_NS430 = false;
		do_not_draw = false;
	}

	// shared_data.track_for_CB[DEF_UNIFORMS_CB_NB] = false;

	a_shared.draw_passed = true;

}


extern "C" {

	// *******************************************************************************************************
	// On draw* : skip draw
	//
	__declspec(dllexport) bool vrem_on_draw(command_list* commandList, uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
	{

		if (!track_for_depthStencil && !do_not_draw && !a_shared.render_effect) return false;
		
		bool skip = false;
		if (do_not_draw) skip = true;

		// log
		log_ondraw(vertex_count, instance_count, first_vertex,first_instance);

		// clear tracking flags
		clear_tracking_flags();

		return skip;
	}

	// *******************************************************************************************************
	// On draw* : skip draw
	__declspec(dllexport) bool vrem_on_draw_indexed(command_list* commandList, uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance)
	{

		if (!track_for_depthStencil && !do_not_draw && !a_shared.render_effect) return false;

		bool skip = false;
		if (do_not_draw) skip = true;

		// log
		log_on_draw_indexed(index_count, instance_count, first_index, vertex_offset, first_instance);

		// clear trackign flags
		clear_tracking_flags();

		return skip;
	}

	// *******************************************************************************************************
	// On draw* : skip draw
	__declspec(dllexport) bool vrem_on_drawOrDispatch_indirect(command_list* commandList, indirect_command type, resource buffer, uint64_t offset, uint32_t draw_count, uint32_t stride)
	{

		if (!track_for_depthStencil && !do_not_draw && !a_shared.render_effect) return false;
		
		bool skip = false;
		if (do_not_draw) skip = true;

		// log
		log_on_drawOrDispatch_indirect(type, buffer, offset, draw_count, stride);

		// clear trackign flags
		clear_tracking_flags();

		return skip;

	}
}