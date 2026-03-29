/**
 *                                  FXAA 3.11
 *
 *                               for ReShade 3.0+
 */

#include "ReShadeUI.fxh"

uniform float Subpix < __UNIFORM_SLIDER_FLOAT1
	ui_min = 0.0; ui_max = 1.0;
	ui_tooltip = "Amount of sub-pixel aliasing removal. Higher values makes the image softer/blurrier.";
> = 0.25;

uniform float EdgeThresholdLuma < __UNIFORM_SLIDER_FLOAT1
	ui_min = 0.0; ui_max = 1.0;
	ui_label = "Edge Detection Threshold (Luma)";
	ui_tooltip = "The minimum amount of local contrast required to apply algorithm.";
> = 0.125;

uniform float EdgeThresholdMin < __UNIFORM_SLIDER_FLOAT1
	ui_min = 0.0; ui_max = 1.0;
	ui_label = "Darkness Threshold";
	ui_tooltip = "Pixels darker than this are not processed in order to increase performance.";
> = 0.0;

uniform bool UseDepthEdges <
    ui_label = "Depth Edge Detection";
    ui_tooltip = "Also apply FXAA on geometry edges detected via depth buffer.";
> = false;

uniform float DepthEdgeThreshold < __UNIFORM_SLIDER_FLOAT1
    ui_min = 0.0001; ui_max = 0.001;
    ui_label = "Depth Edge Threshold";
    ui_tooltip = "Sensitivity of depth-based edge detection.";
	ui_step = 0.0001;
> = 0.001;

uniform int DepthEdgeRadius < __UNIFORM_SLIDER_INT1
    ui_min = 1; ui_max = 4;
    ui_label = "Depth Edge Radius";
    ui_tooltip = "Radius in pixels for depth edge dilation.";
> = 1;

uniform float EdgeThresholdDepth < __UNIFORM_SLIDER_FLOAT1
	ui_min = 0.0; ui_max = 1.0;
	ui_label = "Edge Detection Threshold (depth)";
	ui_tooltip = "The minimum amount of local contrast required to apply algorithm.";
> = 0.125;

uniform bool DebugEdges <
    ui_label = "debug : display FXAA areas";
    ui_tooltip = "Red : area driven by depth, Blue: area driven by Luma";
> = false;

//------------------------------ Non-GUI-settings -------------------------------------------------

#ifndef FXAA_QUALITY__PRESET
	// Valid Quality Presets
	// 10 to 15 - default medium dither (10=fastest, 15=highest quality)
	// 20 to 29 - less dither, more expensive (20=fastest, 29=highest quality)
	// 39       - no dither, very expensive
	#define FXAA_QUALITY__PRESET 15
#endif

#ifndef FXAA_GREEN_AS_LUMA
	#define FXAA_GREEN_AS_LUMA 0
#endif

#ifndef FXAA_LINEAR_LIGHT
	#define FXAA_LINEAR_LIGHT 0
#endif

//-------------------------------------------------------------------------------------------------

#if (__RENDERER__ == 0xb000 || __RENDERER__ == 0xb100)
	#define FXAA_GATHER4_ALPHA 1
	#define FxaaTexAlpha4(t, p) tex2Dgather(t, p, 3)
	#define FxaaTexOffAlpha4(t, p, o) tex2Dgatheroffset(t, p, o, 3)
	#define FxaaTexGreen4(t, p) tex2Dgather(t, p, 1)
	#define FxaaTexOffGreen4(t, p, o) tex2Dgatheroffset(t, p, o, 1)
#endif

#define FXAA_PC 1
#define FXAA_HLSL_3 1

// Green as luma requires non-linear colorspace
#if FXAA_GREEN_AS_LUMA
	#undef FXAA_LINEAR_LIGHT
#endif

#include "FXAA.fxh"
#include "ReShade.fxh"
#include "VREM.fxh"

// Samplers

sampler FXAATexture
{
	Texture = ReShade::BackBufferTex;
	MinFilter = Linear; MagFilter = Linear;
	#if FXAA_LINEAR_LIGHT
		SRGBTexture = true;
	#endif
};

// Pixel shaders

#if !FXAA_GREEN_AS_LUMA
float4 FXAALumaPass(float4 vpos : SV_Position, float2 texcoord : TEXCOORD) : SV_Target
{
	float4 color = tex2D(ReShade::BackBuffer, texcoord.xy);
	color.a = sqrt(dot(color.rgb * color.rgb, float3(0.299, 0.587, 0.114)));
	return color;
}
#endif


bool IsDepthEdge(float2 texcoord)
{
    float depthC = tex2Dlod(DepthBuffer, float4(texcoord, 0, 0)).x;
    float maxDiff = 0.0;

    for (int x = -DepthEdgeRadius; x <= DepthEdgeRadius; x++)
    {
        for (int y = -DepthEdgeRadius; y <= DepthEdgeRadius; y++)
        {
            if (x == 0 && y == 0) continue;
            float2 offset = float2(x * BUFFER_RCP_WIDTH, y * BUFFER_RCP_HEIGHT);
            float depthN  = tex2Dlod(DepthBuffer, float4(texcoord + offset, 0, 0)).x;
            maxDiff = max(maxDiff, abs(depthC - depthN));
        }
    }

    return maxDiff > DepthEdgeThreshold;
}
/*
bool IsDepthEdge(float2 texcoord)
{
    float depthC = ReShade::GetLinearizedDepth(texcoord);
    float maxDiff = 0.0;

    for (int x = -DepthEdgeRadius; x <= DepthEdgeRadius; x++)
    {
        for (int y = -DepthEdgeRadius; y <= DepthEdgeRadius; y++)
        {
            if (x == 0 && y == 0) continue;

            float2 offset = float2(x * BUFFER_RCP_WIDTH, y * BUFFER_RCP_HEIGHT);
            float depthN  = ReShade::GetLinearizedDepth(texcoord + offset);
            maxDiff = max(maxDiff, abs(depthC - depthN));
        }
    }

    return maxDiff > DepthEdgeThreshold;
}
*/

#ifndef DEPTH_EDGE_RADIUS
    #define DEPTH_EDGE_RADIUS 2
#endif

bool IsCockpitEdge(float2 texcoord)
{
    bool cIsCockpit = (mak_value(tex2D(StencilBuffer, texcoord), tex2D(DepthBuffer, texcoord)) == COCKPIT);

    #define COCKPIT_CHECK(ox, oy) \
        if (cIsCockpit != (mak_value(tex2D(StencilBuffer, texcoord + float2((ox) * BUFFER_RCP_WIDTH, (oy) * BUFFER_RCP_HEIGHT)), \
                                     tex2D(DepthBuffer,   texcoord + float2((ox) * BUFFER_RCP_WIDTH, (oy) * BUFFER_RCP_HEIGHT))) == COCKPIT)) return true;

    // Radius 1 - cardinaux + diagonales
    COCKPIT_CHECK( 1,  0) COCKPIT_CHECK(-1,  0)
    COCKPIT_CHECK( 0,  1) COCKPIT_CHECK( 0, -1)
    COCKPIT_CHECK( 1,  1) COCKPIT_CHECK(-1,  1)
    COCKPIT_CHECK( 1, -1) COCKPIT_CHECK(-1, -1)

#if DEPTH_EDGE_RADIUS >= 2
    COCKPIT_CHECK( 2,  0) COCKPIT_CHECK(-2,  0)
    COCKPIT_CHECK( 0,  2) COCKPIT_CHECK( 0, -2)
    COCKPIT_CHECK( 2,  1) COCKPIT_CHECK(-2,  1)
    COCKPIT_CHECK( 2, -1) COCKPIT_CHECK(-2, -1)
    COCKPIT_CHECK( 1,  2) COCKPIT_CHECK(-1,  2)
    COCKPIT_CHECK( 1, -2) COCKPIT_CHECK(-1, -2)
    COCKPIT_CHECK( 2,  2) COCKPIT_CHECK(-2,  2)
    COCKPIT_CHECK( 2, -2) COCKPIT_CHECK(-2, -2)
#endif

#if DEPTH_EDGE_RADIUS >= 3
    COCKPIT_CHECK( 3,  0) COCKPIT_CHECK(-3,  0)
    COCKPIT_CHECK( 0,  3) COCKPIT_CHECK( 0, -3)
    COCKPIT_CHECK( 3,  1) COCKPIT_CHECK(-3,  1) COCKPIT_CHECK( 3, -1) COCKPIT_CHECK(-3, -1)
    COCKPIT_CHECK( 3,  2) COCKPIT_CHECK(-3,  2) COCKPIT_CHECK( 3, -2) COCKPIT_CHECK(-3, -2)
    COCKPIT_CHECK( 3,  3) COCKPIT_CHECK(-3,  3) COCKPIT_CHECK( 3, -3) COCKPIT_CHECK(-3, -3)
    COCKPIT_CHECK( 1,  3) COCKPIT_CHECK(-1,  3) COCKPIT_CHECK( 1, -3) COCKPIT_CHECK(-1, -3)
    COCKPIT_CHECK( 2,  3) COCKPIT_CHECK(-2,  3) COCKPIT_CHECK( 2, -3) COCKPIT_CHECK(-2, -3)
#endif

    #undef COCKPIT_CHECK
    return false;
}

bool IsPlaneFar(float2 texcoord)
{
    bool cIsPlane = (mak_value(tex2D(MaskBuffer, texcoord), tex2D(DepthBuffer, texcoord)) == PLANE);
	/*
	float4 mask = tex2Dlod(MaskBuffer, float4(texcoord, 0, 0));
	float4 depth = tex2Dlod(DepthBuffer, float4(texcoord, 0, 0));
	int area = mak_value(mask, depth);
	if (area == PLANE) return true;
	*/

    #define PLANE_CHECK(ox, oy) \
        if (cIsPlane != (mak_value(tex2D(MaskBuffer, texcoord + float2((ox) * BUFFER_RCP_WIDTH, (oy) * BUFFER_RCP_HEIGHT)), \
                                     tex2D(DepthBuffer,   texcoord + float2((ox) * BUFFER_RCP_WIDTH, (oy) * BUFFER_RCP_HEIGHT))) == PLANE)) return true;

    // Radius 1 - cardinaux + diagonales
    PLANE_CHECK( 1,  0) PLANE_CHECK(-1,  0)
    PLANE_CHECK( 0,  1) PLANE_CHECK( 0, -1)
    PLANE_CHECK( 1,  1) PLANE_CHECK(-1,  1)
    PLANE_CHECK( 1, -1) PLANE_CHECK(-1, -1)

#if DEPTH_EDGE_RADIUS >= 2
    PLANE_CHECK( 2,  0) PLANE_CHECK(-2,  0)
    PLANE_CHECK( 0,  2) PLANE_CHECK( 0, -2)
    PLANE_CHECK( 2,  1) PLANE_CHECK(-2,  1)
    PLANE_CHECK( 2, -1) PLANE_CHECK(-2, -1)
    PLANE_CHECK( 1,  2) PLANE_CHECK(-1,  2)
    PLANE_CHECK( 1, -2) PLANE_CHECK(-1, -2)
    PLANE_CHECK( 2,  2) PLANE_CHECK(-2,  2)
    PLANE_CHECK( 2, -2) PLANE_CHECK(-2, -2)
#endif

#if DEPTH_EDGE_RADIUS >= 3
    PLANE_CHECK( 3,  0) PLANE_CHECK(-3,  0)
    PLANE_CHECK( 0,  3) PLANE_CHECK( 0, -3)
    PLANE_CHECK( 3,  1) PLANE_CHECK(-3,  1) PLANE_CHECK( 3, -1) PLANE_CHECK(-3, -1)
    PLANE_CHECK( 3,  2) PLANE_CHECK(-3,  2) PLANE_CHECK( 3, -2) PLANE_CHECK(-3, -2)
    PLANE_CHECK( 3,  3) PLANE_CHECK(-3,  3) PLANE_CHECK( 3, -3) PLANE_CHECK(-3, -3)
    PLANE_CHECK( 1,  3) PLANE_CHECK(-1,  3) PLANE_CHECK( 1, -3) PLANE_CHECK(-1, -3)
    PLANE_CHECK( 2,  3) PLANE_CHECK(-2,  3) PLANE_CHECK( 2, -3) PLANE_CHECK(-2, -3)
#endif

    #undef PLANE_CHECK
	
    return false;
}

float4 FXAAPixelShader(float4 vpos : SV_Position, float2 texcoord : TEXCOORD) : SV_Target
{
	
	float4 original = tex2D(FXAATexture, texcoord);
	float4 fxaa = original;

	float4 depth = tex2Dlod(DepthBuffer, float4(texcoord, 0, 0));
    bool depth_edge = UseDepthEdges && IsDepthEdge(texcoord);
	
	uint fxaa_mode = 0;
	
	float EdgeThreshold;
	
	bool planeFar = IsPlaneFar(texcoord);
	
	if (!planeFar)
	{
		
		bool cockpit_edge = IsCockpitEdge(texcoord);
		
		if (cockpit_edge && UseDepthEdges)
		{
			fxaa_mode = 2;
			EdgeThreshold = EdgeThresholdLuma;
		}
		else
		{
			
			// case 1: depth.x >= 0.001 and in depth edge : own plane, edge only
			if (depth.x >= 0.001 && UseDepthEdges && depth_edge) 
			{
				fxaa_mode = 1;
				EdgeThreshold = EdgeThresholdDepth;
			}
			// case 2: depth.x < 0.001 and not in depth edge : other parts except objects, luma only
			if ((depth.x < 0.001 && !depth_edge) || !UseDepthEdges) 
			{
				fxaa_mode = 2;
				EdgeThreshold = EdgeThresholdLuma;
			}
			
			// case 3: depth.x < 0.001 and not in depth edge : other parts except objects, luma only
			if ((depth.x < 0.001 && !depth_edge) || !UseDepthEdges) 
			{
				fxaa_mode = 2;
				EdgeThreshold = EdgeThresholdLuma;
			}
		}
	}	

	if (fxaa_mode)
	{
		
	
		fxaa = FxaaPixelShader(
			texcoord, // pos
			0, // fxaaConsolePosPos
			FXAATexture, // tex
			FXAATexture, // fxaaConsole360TexExpBiasNegOne
			FXAATexture, // fxaaConsole360TexExpBiasNegTwo
			BUFFER_PIXEL_SIZE, // fxaaQualityRcpFrame
			0, // fxaaConsoleRcpFrameOpt
			0, // fxaaConsoleRcpFrameOpt2
			0, // fxaaConsole360RcpFrameOpt2
			Subpix, // fxaaQualitySubpix
			EdgeThreshold, // fxaaQualityEdgeThreshold
			EdgeThresholdMin, // fxaaQualityEdgeThresholdMin
			0, // fxaaConsoleEdgeSharpness
			0, // fxaaConsoleEdgeThreshold
			0, // fxaaConsoleEdgeThresholdMin
			0 // fxaaConsole360ConstDir
		);
		
		
		if (DebugEdges)
		{
			
			//bool color_change  = length(fxaa.rgb - original.rgb) > 0.00001;

			bool color_change  = length(fxaa.rgb - original.rgb) != 0;
			if (color_change && fxaa_mode == 1) return float4(1.0, 0.0, 0.0, 1.0); // red = luma
			if (color_change &&  fxaa_mode == 2) return float4(0.0, 0.0, 1.0, 1.0); // bleu   = profondeur seule

		}
		
		/*
		if (DebugEdges)
		{	
			if (planeFar) return float4(1.0, 0.0, 0.0, 1.0);
		}
		*/
	}


    return fxaa;
}

// Rendering passes

technique VREM_SmartFXAA
{
#if !FXAA_GREEN_AS_LUMA
	pass
	{
		VertexShader = PostProcessVS;
		PixelShader = FXAALumaPass;
	}
#endif
	pass
	{
		VertexShader = PostProcessVS;
		PixelShader = FXAAPixelShader;
		#if FXAA_LINEAR_LIGHT
			SRGBWriteEnable = true;
		#endif
	}
}
