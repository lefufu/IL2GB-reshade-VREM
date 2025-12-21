///////////////////////////////////////////////////////////////////////
//
// Reshade IL2 VREM addon. VR Enhancer Mod for IL2 using reshade
// "hot" reload of mod possible using a Reshade addon as launcher (loaded with the game)
// and a dll containing the mod logic itselve. Mod settings are in uniforms of a technique
// 
// ----------------------------------------------------------------------------------------
//  clone pipeline and add reference of the cloned pipeline in the original 
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
#include "config.hpp"
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>

#include "addon_objects.h"
#include "addon_logs.h"

extern std::unordered_map<uint32_t, std::vector<uint8_t>> shader_code_cache;
std::unordered_map<uint64_t, reshade::api::pipeline> cloned_pipeline_list;

using namespace reshade::api;


// *******************************************************************************************************
/// <summary>
/// clone pipeline with a shader and replace code by the modded one
/// </summary>

pipeline clone_pipeline(
    device* device,
    reshade::api::pipeline_layout layout,
    uint32_t subobjectCount,
    const reshade::api::pipeline_subobject* subobjects,
    reshade::api::pipeline pipeline,
    uint32_t hash[])
{

    /*struct CachedShader {
        void* data = nullptr;
        size_t size = 0;
    };*/

    reshade::api::pipeline pipelineClone = {};

    // clone subobjects
    reshade::api::pipeline_subobject* newSubobjects = new reshade::api::pipeline_subobject[subobjectCount];
    memcpy(newSubobjects, subobjects, sizeof(reshade::api::pipeline_subobject) * subobjectCount);


    // clone the desc and change code source
    for (uint32_t i = 0; i < subobjectCount; ++i) {
        auto clonedSubObject = &newSubobjects[i];
        //CachedShader* cache;

        auto it = shader_code_cache.find(hash[i]);
        if (it == shader_code_cache.end())
        {
            // Log error: shader not found in cache
            return pipelineClone; // Retourne un pipeline invalide
        }

        const std::vector<uint8_t>& shader_code = it->second;

        //original Desc
        shader_desc desc = *static_cast<shader_desc*>(subobjects[i].data);

        // Clone desc
        reshade::api::shader_desc clonedDesc;
        memcpy(&clonedDesc, &desc, sizeof(reshade::api::shader_desc));

        // Point to cloned desc
        clonedSubObject->data = &clonedDesc;

        // change code source to use the new one 
        // clone ReplaceshaderCode
        clonedDesc.code = shader_code.data();
        clonedDesc.code_size = shader_code.size();

        // create cloned pipeline

        bool builtPipelineOK = device->create_pipeline(
            layout,
            subobjectCount,
            &newSubobjects[0],
            &pipelineClone
        );

        if (builtPipelineOK) {
            log_pipeline_clone_OK(pipeline.handle, pipelineClone.handle);
        }
        else
        {
            /*
            // log error
            s << "!!! Error in cloning pipeline !!! ("
                << ", orig pipeline: " << reinterpret_cast<void*>(pipeline.handle)
                << ")";
            reshade::log::message(reshade::log::level::info, s.str().c_str());
            */
        }

        // free allocated memory as the shader is created or failed
        // free(cache->data);
        delete[] newSubobjects;
    }
    return pipelineClone;
}

// *******************************************************************************************************
/// <summary>
/// delete cloned pipeline with a shader and replace code by the modded one
/// </summary>

void delete_cloned_pipelines(reshade::api::device* dev)
{
    for (auto& [handle, pipeline] : cloned_pipeline_list)
    {
		// check if pipeline is valid before destroying
        if (pipeline.handle != 0)
        {
            dev->destroy_pipeline(pipeline);
        }
    }

    // clean the map
    cloned_pipeline_list.clear();
}