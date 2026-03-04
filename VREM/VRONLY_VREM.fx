//
// debug shader for VREM : display depth or stencil buffer
//
// 2025/01 							Lefuneste.
//
// https://forum.dcs.world/topic/356128-reshade-vr-enhancer-mod-vrem/#comment-5503216

#include "ReShade.fxh"
#include "ReShadeUI.fxh"


uniform string testMessage < ui_label = "Technique used only in VR"; ui_text = "!!! setup by VREM for 'technique in VR only', do not tick it !!!"; > = "xx";


technique VREM_technique_in_VR_only
{

}
