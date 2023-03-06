#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <memory>
#include <unordered_map>

#include "Camera.h"
#include "Transform.h"
#include "DX12Helper.h"

enum MaterialType {
	Normal,
	Transparent,
	Emissive
};

class Material
{
public:
	Material(
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState,
		//defualt fully metal
		DirectX::XMFLOAT4 tint = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
		MaterialType type = MaterialType::Normal,
		DirectX::XMFLOAT2 uvScale = DirectX::XMFLOAT2(1, 1),
		DirectX::XMFLOAT2 uvOffset = DirectX::XMFLOAT2(0, 0));

	DirectX::XMFLOAT2 GetUVScale();
	DirectX::XMFLOAT2 GetUVOffset();
	DirectX::XMFLOAT4 GetColorTint();
	MaterialType GetType();

	void SetUVScale(DirectX::XMFLOAT2 scale);
	void SetUVOffset(DirectX::XMFLOAT2 offset);
	void SetColorTint(DirectX::XMFLOAT4 tint);
	void SetType(MaterialType type);

	void AddTexture(D3D12_CPU_DESCRIPTOR_HANDLE srv, int slot);
	//done adding textures
	void FinalizeMaterial();

	//getters for pipelinestate and gpu handle
	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPipelineState();
	D3D12_GPU_DESCRIPTOR_HANDLE GetFinalGPUHandleForTextures();

private:
	bool finalized = false;

	// Material properties
	DirectX::XMFLOAT4 colorTint;
	MaterialType type;

	// Texture-related
	DirectX::XMFLOAT2 uvOffset;
	DirectX::XMFLOAT2 uvScale;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
	
	const int numTexSlots = 4;

	//array to hold the four tex types we'll need
	//albedo normal, metalness, roughness in that order
	D3D12_CPU_DESCRIPTOR_HANDLE textureSRVsBySlot[4];

	//location of first srv in heap
	D3D12_GPU_DESCRIPTOR_HANDLE finalGPUHandleForSRVs;
};

