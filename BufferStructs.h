#pragma once
#include<DirectXMath.h>
#include "Lights.h"

//must match vs cbuffer layout exactly
struct VertexShaderExternalData {
	DirectX::XMFLOAT4X4 world;
	DirectX::XMFLOAT4X4 worldInverseTranspose;
	DirectX::XMFLOAT4X4 view;
	DirectX::XMFLOAT4X4 proj;
};

// Alignment matters!!!
// must match alignment in pixel shader
struct PixelShaderExternalData {
	DirectX::XMFLOAT2 uvScale;
	DirectX::XMFLOAT2 uvOffset;
	DirectX::XMFLOAT3 cameraPosition;
	float padding;
	DirectX::XMFLOAT3 colorTint;
	int lightCount;
	Light lights[MAX_LIGHTS];
};

//world scene data, all rays need it
struct RaytracingSceneData {
	DirectX::XMFLOAT4X4 inverseViewProjection;
	DirectX::XMFLOAT3 cameraPosition;
	unsigned int raysPerPixel;
	unsigned int maxRecursion;
};

//must match raytracing shader define
#define MAX_INSTANCES_PER_BLAS 100
struct RaytracingEntityData {
	DirectX::XMFLOAT4 color[MAX_INSTANCES_PER_BLAS];
};