#include "Structs.hlsli"
#include "Lighting.hlsli"

// Alignment matters!!!
cbuffer ExternalData : register(b0)
{
	float2 uvScale;
	float2 uvOffset;
	float3 cameraPosition;
	float padding;
	float3 colorTint;
	int lightCount;
	Light lights[MAX_LIGHTS];
}

//registers for textures
Texture2D AlbedoTex : register(t0);
Texture2D MetalTex : register(t1);
Texture2D NormalTex : register(t2);
Texture2D RoughnessTex : register(t3);

SamplerState BasicSampler : register(s0);

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
	// Always re-normalize interpolated direction vectors
	input.normal = normalize(input.normal);
	input.tangent = normalize(input.tangent);

	// Apply the uv adjustments
	input.uv = input.uv * uvScale + uvOffset;

	// Sample various textures
	input.normal = NormalMapping(NormalTex, BasicSampler, input.uv, input.normal, input.tangent);
	float roughness = RoughnessTex.Sample(BasicSampler, input.uv).r;
	float metal = MetalTex.Sample(BasicSampler, input.uv).r;

	// Gamma correct the texture back to linear space and apply the color tint
	float4 surfaceColor = AlbedoTex.Sample(BasicSampler, input.uv);
	surfaceColor.rgb = pow(surfaceColor.rgb, 2.2) * colorTint;
	//return float4(colorTint, 1.0f);

	// Specular color - Assuming albedo texture is actually holding specular color if metal == 1
	// Note the use of lerp here - metal is generally 0 or 1, but might be in between
	// because of linear texture sampling, so we want lerp the specular color to match
	float3 specColor = lerp(F0_NON_METAL.rrr, surfaceColor.rgb, metal);

	// Total color for this pixel
	float3 totalColor = float3(0,0,0);

	// Loop through all lights this frame
	for (int i = 0; i < lightCount; i++)
	{
		// Which kind of light?
		switch (lights[i].Type)
		{
		case LIGHT_TYPE_DIRECTIONAL:
			totalColor += DirLightPBR(lights[i], input.normal, input.worldPos, cameraPosition, roughness, metal, surfaceColor.rgb, specColor);
			break;

		case LIGHT_TYPE_POINT:
			totalColor += PointLightPBR(lights[i], input.normal, input.worldPos, cameraPosition, roughness, metal, surfaceColor.rgb, specColor);
			break;

		case LIGHT_TYPE_SPOT:
			totalColor += SpotLightPBR(lights[i], input.normal, input.worldPos, cameraPosition, roughness, metal, surfaceColor.rgb, specColor);
			break;
		}
	}

	//totalColor = surfaceColor.rgb;
	//return float4(lights[0].Color.b,0,0, 1.0f);

	// Gamma correction
	return float4(pow(totalColor, 1.0f / 2.2f), 1);	
}