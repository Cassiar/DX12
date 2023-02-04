#pragma once
#include<DirectXMath.h>

//must match vs cbuffer layout exactly
struct VertexShaderExternalData {
	DirectX::XMFLOAT4X4 world;
	DirectX::XMFLOAT4X4 worldInverseTranspose;
	DirectX::XMFLOAT4X4 view;
	DirectX::XMFLOAT4X4 proj;
};