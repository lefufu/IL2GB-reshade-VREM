#include <unordered_map>
#include <string>

extern bool addon_init;
extern float VREM_setting[];
extern std::unordered_map<std::string, int> settings_mapping;
extern SharedState* g_shared_state;

// mod setting

constexpr uint8_t PARAMS_NB = 10;
constexpr uint8_t FPS_LIMIT = 0;
constexpr uint8_t DUMMY = 1;



/*std::unordered_map<std::string, int> settings_mapping = {
    {"fps_limit", FPS_LIMIT},
    {"flag_fps", DUMMY}
};
*/
