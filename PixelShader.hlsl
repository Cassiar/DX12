#include "Structs.hlsli"
#include "Lighting.hlsli"

// Alignment matters!!!
cbuffer ExternalData : register(b0)
{
	float2 uvScale;
	float2 uvOffset;
	float3 cameraPosition;
	int lightCount;
	Light lights[MAX_LIGHTS];
}

//registers for textures
Texture2D albedoTex : register(t0);
Texture2D metalTex : register(t1);
Texture2D normalTex : register(t2);
Texture2D roughnessTex : register(t3);

SamplerState basicSampler : register(s0);

// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// 
// - Input is the data coming down the pipeline (defined by the struct)
// - Output is a single color (float4)
// - Has a special semantic (SV_TARGET), which means 
//    "put the output of this into the current render target"
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
	// Just return the input color
	// - This color (like most values passing through the rasterizer) is 
	//   interpolated for each pixel between the corresponding vertices 
	//   of the triangle we're rendering
	float4 finalColor = 0;

	finalColor += albedoTex.Sample(basicSampler, input.uv);

	return finalColor;
}