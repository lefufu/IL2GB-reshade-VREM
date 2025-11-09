#include <reshade.hpp>
#include <filesystem>

#include "loader_addon_shared.h"
#include "VREM_settings.h"


// static SharedState* g_shared_state = nullptr;

extern "C" {

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
}
