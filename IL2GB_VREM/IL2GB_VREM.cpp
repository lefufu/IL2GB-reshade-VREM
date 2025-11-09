/*
 * Copyright (C) 2022 Patrick Mours
 * SPDX-License-Identifier: BSD-3-Clause OR MIT
 */

// #include <imgui.h>
#include <reshade.hpp>
#include <chrono>
#include <thread>
#include <filesystem>

#include "loader_addon_shared.h"
#include "VREM_settings.h"

static PersistentPipelineData* g_persistent = nullptr;
// static SharedState* g_shared_state = nullptr;

// static bool addon_init = false;
bool addon_init = false;
SharedState* g_shared_state = nullptr;

static int s_fps_limit = 0;
static std::chrono::high_resolution_clock::time_point s_last_time_point;

float VREM_setting[PARAMS_NB];

std::unordered_map<std::string, int> settings_mapping = {
    {"fps_limit", FPS_LIMIT},
    {"flag_fps", DUMMY}
};

using namespace reshade::api;

// Structure pour stocker les informations d'un uniform
struct UniformInfo {
    std::string name;
    effect_uniform_variable variable;
    std::vector<uint8_t> data;

    bool operator!=(const UniformInfo& other) const {
        return data.size() != other.data.size() ||
            std::memcmp(data.data(), other.data.data(), data.size()) != 0;
    }
};




// État global
static std::unordered_map<std::string, UniformInfo> previous_uniforms;
static bool overlay_is_open = false;
static bool was_open_last_frame = false;

// retrieve VREM settings from uniform of VREM_settings technique

void get_settings_from_uniforms(effect_runtime* runtime) {
    std::unordered_map<std::string, UniformInfo> uniforms;

    std::string name = "VREM_settings.fx";
    const char* effect_name = name.c_str();

    reshade::log::message(reshade::log::level::info,
        "get_settings_from_uniforms() called");

    runtime->enumerate_uniform_variables(effect_name,
        [&](effect_runtime* rt, effect_uniform_variable var) {
            char name_buffer[256];
            rt->get_uniform_variable_name(var, name_buffer);
            std::string uniform_name(name_buffer);

            rt->get_uniform_variable_effect_name(var, name_buffer);
            std::string effect_name(name_buffer);

            UniformInfo info;
            info.name = uniform_name;
            info.variable = var;
            format uniform_format;
            uint32_t out_row, out_colomns, out_array_length;
            float uniform_value = 0;

            rt->get_uniform_variable_type(var, &uniform_format, &out_row, &out_colomns, &out_array_length);
            if (out_array_length > 1)
            {
                std::stringstream s;
                s << "effect name=" << effect_name << ", uniform name = " << uniform_name;
                s << "array_length > 1 :" << out_array_length << ";";
                reshade::log::message(reshade::log::level::info, s.str().c_str());

            } else
            { 
                switch (uniform_format)
                {
                    case format::r32_typeless:
                    {
                        bool value;
                        rt->get_uniform_value_bool(var, &value, 1, 0);
                        uniform_value = value;
                        break;
                    }

                    case format::r32_uint:
                    {
                        uint32_t value;
                        rt->get_uniform_value_uint(var, &value, 1, 0);
                        uniform_value = value;
                        break;
                    }

                    case format::r32_sint:
                    {
                        int32_t value;
                        rt->get_uniform_value_int(var, &value, 1, 0);
                        uniform_value = value;
                        break;
                    }

                    case format::r32_float:
                    {
                        float value;
                        rt->get_uniform_value_float(var, &value, 1, 0);
                        uniform_value = value;
                        break;

                    }
                }
            }
            /*
            std::stringstream s;
            s << "effect name=" << effect_name  << ", uniform name = " << uniform_name << ", uniform value =" << uniform_value << "; ";
            reshade::log::message(reshade::log::level::info, s.str().c_str());
            */
            

            // update vrem settings
            if (settings_mapping.count(uniform_name) > 0)
            {
                VREM_setting[settings_mapping[uniform_name]] = uniform_value;
            }

        });
}


extern "C" {

    /*
    __declspec(dllexport) void vrem_init(
        reshade::api::device* device,
        reshade::api::command_queue* queue,
        reshade::api::swapchain* swapchain, 
        PersistentPipelineData* persistent_data,
        SharedState* shared_state
    )
    {
        reshade::log::message(reshade::log::level::info,
            "** DCS VREM: Initialisation de l'addon...");

        // Code du DLL_PROCESS_ATTACH
        WCHAR buf[MAX_PATH];
        const std::filesystem::path dllPath = GetModuleFileNameW(nullptr, buf, ARRAYSIZE(buf)) ?
            buf : std::filesystem::path();
        const std::filesystem::path basePath = dllPath.parent_path();
        //
        //reshade::register_event<reshade::addon_event::present>(on_present);
        //reshade::register_overlay(nullptr, draw_settings);

        g_shared_state = shared_state;

        g_shared_state->s_last_time_point = std::chrono::high_resolution_clock::now();

        addon_init = true;

        reshade::log::message(reshade::log::level::info,
            "DCS VREM: register done...");
    }

    __declspec(dllexport) void vrem_cleanup(PersistentPipelineData* persistent_data)
    {
        reshade::log::message(reshade::log::level::info,
            "DCS VREM: Nettoyage de l'addon...");


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
    */
    /*
    _declspec(dllexport) void draw_settings(reshade::api::effect_runtime*)
    {
        
        reshade::log::message(reshade::log::level::info,
            "Call draw_settings");

        // CRITIQUE : Vérifier que g_shared_state est initialisé
        if (!g_shared_state) {
            reshade::log::message(reshade::log::level::warning,
                "draw_settings: g_shared_state is nullptr! Skipping...");
            return;
        }
        
        if (ImGui::DragInt("Target FPS", &g_shared_state->s_fps_limit, 1, 0, 200))
            g_shared_state->s_last_time_point = std::chrono::high_resolution_clock::now();

        ImGui::SetItemTooltip("Set to zero to disable the FPS limit.");

        reshade::log::message(reshade::log::level::info,
            "End of draw_settings call");
    }
    */
    

    _declspec(dllexport) void vrem_on_reshade_present(reshade::api::effect_runtime* runtime)
    {
 
        // to get VREM settings => read from uniform values

        // auto current_uniforms = get_all_uniforms(runtime);

        /**** Error when reading g_shared_state->overlay_is_open *****/
        if (g_shared_state->overlay_is_open || addon_init == true) {
        // if (g_shared_state->overlay_is_open) {
        // if (addon_init == true) {

            // Récupérer l'état actuel des uniforms
            get_settings_from_uniforms(runtime);
            addon_init = false;

        }

        //g_shared_state->was_open_last_frame = g_shared_state->overlay_is_open;
        // g_shared_state->overlay_is_open = false; // Réinitialiser (sera remis à true si l'overlay est ouvert)


        // fps limiter function
        // if (g_shared_state->s_fps_limit <= 0)
        if (VREM_setting[DUMMY] == 0)
            return;

        //reshade::log::message(reshade::log::level::info,
        //    "vrem_on_reshade_present called");

        /*
        if (g_shared_state->last_fps_limit != VREM_setting[FPS_LIMIT])
        {
            g_shared_state->s_last_time_point = std::chrono::high_resolution_clock::now();
            g_shared_state->last_fps_limit = VREM_setting[FPS_LIMIT];
        }
        */


        const auto time_per_frame = std::chrono::high_resolution_clock::duration(std::chrono::seconds(1)) / int(VREM_setting[FPS_LIMIT]);
        const auto next_time_point = g_shared_state->s_last_time_point + time_per_frame;

        while (next_time_point > std::chrono::high_resolution_clock::now())
            std::this_thread::sleep_for(std::chrono::high_resolution_clock::duration(std::chrono::milliseconds(1)));

        g_shared_state->s_last_time_point = next_time_point;

    }

    // Callback appelé quand les effets sont rechargés
    _declspec(dllexport) void vrem_on_reshade_reloaded_effects(effect_runtime* runtime)
    {

        reshade::log::message(reshade::log::level::info,
            "Effets rechargés - réinitialisation du cache des uniforms");

        // Recharger tous les uniforms dans le cache
        get_settings_from_uniforms(runtime);
    }


}
