//
// debug shader for VREM : display depth or stencil buffer
//
// 2025/01 							Lefuneste.
//
// https://forum.dcs.world/topic/356128-reshade-vr-enhancer-mod-vrem/#comment-5503216

#include "ReShade.fxh"
#include "ReShadeUI.fxh"

#include "VREM.fxh"

//****************************************
// GUI

uniform float unif_test;

uniform int display_mode <
    ui_type = "combo";
    ui_label = "Mode";
    ui_tooltip = "Choose a texture to display";
    ui_category = "Debug Options";
    ui_items = 
    "Nothing\0"
	"Depth\0"
    "Mask\0"
	"Stencil\0"
    ;
> = 0;


uniform float EdgeThreshold <
    ui_type = "slider";
    ui_label = "Seuil d'arête";
    ui_min = 0.0; ui_max = 1.0; ui_step = 0.001;
> = 0.05;

uniform float EdgeThicknessScale <
    ui_type = "slider";
    ui_label = "Épaisseur (scale du texel)";
    ui_min = 0.5; ui_max = 4.0; ui_step = 0.1;
> = 1.0;

uniform float4 EdgeColor <
    ui_type = "color";
    ui_label = "Couleur des arêtes";
> = float4(0.0, 0.0, 0.0, 1.0);

uniform int EdgeMethod <
    ui_type = "combo";
    ui_label = "Méthode";
    ui_items = "Sobel\0Roberts Cross\0Laplacien\0";
> = 0;

float GetDepth(float2 texcoord)
{
    // Retourne une profondeur linéarisée [0..1]
    //float4 depth = 10*tex2Dlod(DepthBuffer, float4(texcoord, 0, 0));
	//return depth.x;
	
	return 5*ReShade::GetLinearizedDepth(texcoord);
}

float SobelEdge(float2 uv, float2 texelSize)
{
    float tl = GetDepth(uv + texelSize * float2(-1, -1));
    float tc = GetDepth(uv + texelSize * float2( 0, -1));
    float tr = GetDepth(uv + texelSize * float2( 1, -1));
    float ml = GetDepth(uv + texelSize * float2(-1,  0));
    float mr = GetDepth(uv + texelSize * float2( 1,  0));
    float bl = GetDepth(uv + texelSize * float2(-1,  1));
    float bc = GetDepth(uv + texelSize * float2( 0,  1));
    float br = GetDepth(uv + texelSize * float2( 1,  1));

    float gx = -tl - 2.0*ml - bl + tr + 2.0*mr + br;
    float gy = -tl - 2.0*tc - tr + bl + 2.0*bc + br;
    return sqrt(gx*gx + gy*gy);
}

float RobertsEdge(float2 uv, float2 texelSize)
{
    float d00 = GetDepth(uv);
    float d10 = GetDepth(uv + texelSize * float2(1, 0));
    float d01 = GetDepth(uv + texelSize * float2(0, 1));
    float d11 = GetDepth(uv + texelSize * float2(1, 1));

    return abs(d00 - d11) + abs(d10 - d01);
}

float LaplacianEdge(float2 uv, float2 texelSize)
{
    float c = GetDepth(uv);
    float n = GetDepth(uv + texelSize * float2( 0, -1));
    float s = GetDepth(uv + texelSize * float2( 0,  1));
    float e = GetDepth(uv + texelSize * float2( 1,  0));
    float w = GetDepth(uv + texelSize * float2(-1,  0));

    return abs(4.0 * c - n - s - e - w);
}

uniform int bufferWidth < ui_type = "drag"; ui_label = "BUFFER Width"; ui_min = 0; ui_max = 4096; > = BUFFER_WIDTH;
uniform int bufferHeight < ui_type = "drag"; ui_label = "BUFFER Height"; ui_min = 0; ui_max = 4096; > = BUFFER_HEIGHT;
/*
uniform int QVWidth < ui_type = "drag"; ui_label = "QV Width"; ui_min = 1076; ui_max = 1660; > = 1076;
uniform int QVHeight < ui_type = "drag"; ui_label = "QV Height"; ui_min = 1076; ui_max = 1420; > = 1076;
*/
//****************************************
// code

// pixel shader
float3 Ps_VREM_Test(float4 position : SV_Position, float2 texcoord : TEXCOORD) : SV_Target
{
	float2 textcoord2;
	
	float3 ori = tex2Dlod(ReShade::BackBuffer, float4(texcoord, 0.0, 0.0)).rgb;
	float4 debug;
	
	float3 output ;
	
	if (!display_mode)
	{
		return ori;
	}
	
	else if (display_mode == 1)
	{	
		
		float4 depth = tex2Dlod(DepthBuffer, float4(texcoord, 0, 0));
		
		//depth + stencil (sight)
		output = float3(depth.x*10, 0, 0);
		if (depth.y != 0.0) output.y = 1.0;
		
		//try to compute edge
		float2 texelSize = ReShade::PixelSize * EdgeThicknessScale;

		float edge = 0.0;

		if (EdgeMethod == 0)
			edge = SobelEdge(texcoord, texelSize);
		else if (EdgeMethod == 1)
			edge = RobertsEdge(texcoord, texelSize);
		else
			edge = LaplacianEdge(texcoord, texelSize);

		// Seuillage
		float mask = step(EdgeThreshold, edge);

		// Composite : arête par-dessus la couleur originale
		return lerp(ori, EdgeColor, mask * EdgeColor.a);
			
	}
		// mask
		if (display_mode == 2)
		{
			
				float4 mask = tex2Dlod(MaskBuffer, float4(texcoord, 0, 0));
				float4 depth = tex2Dlod(DepthBuffer, float4(texcoord, 0, 0));
				int area = mak_value(mask, depth);
				
				switch (area)
				{
					case AIRFRAME:
						debug.xyz = float3(0.5, 0.0, 0);
						break;
					
					case COCKPIT:
						debug.xyz = float3(1.0, 0.0, 0);
						break;
					
					case SKY:
						debug.xyz = float3(0, 0, 1.0);
						break;
					
					case WATER:
						debug.xyz = float3(0, 0, 0.5);
						break;
					
					case GROUND:
						debug.xyz = float3(0.0, 1.0, 0);
						break;
					
					case PLANE:
						debug.xyz = float3(1, 1, 0);
						break;
					
					default:
						debug.xyz = float3(0, 0, 0);
						break;
				
				}
			
			output.xyz = debug.xyz;
		}
		else if (display_mode == 3)
		{
			if (is_sight(texcoord))
			{
				output = float3(1,0,0);
			} else
				output = ori;
		}
		else 
		{
			output = ori;
		}
	
	//if (unif_test == 1.0) output.x += 0.5;
	//output.x =1.0;
	
	return output;
	
}


technique VREM_Test_masks
{
    pass
    {
		VertexShader = PostProcessVS;
        PixelShader = Ps_VREM_Test; 
    }
}
