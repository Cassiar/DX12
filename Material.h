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

class Material
{
public:
	Material(
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState,
		DirectX::XMFLOAT3 tint,
		DirectX::XMFLOAT2 uvScale = DirectX::XMFLOAT2(1, 1),
		DirectX::XMFLOAT2 uvOffset = DirectX::XMFLOAT2(0, 0));

	DirectX::XMFLOAT2 GetUVScale();
	DirectX::XMFLOAT2 GetUVOffset();
	DirectX::XMFLOAT3 GetColorTint();

	void SetUVScale(DirectX::XMFLOAT2 scale);
	void SetUVOffset(DirectX::XMFLOAT2 offset);
	void SetColorTint(DirectX::XMFLOAT3 tint);

	void AddTexture(D3D12_CPU_DESCRIPTOR_HANDLE srv, int slot);
	//done adding textures
	void FinalizeMaterial();

	//getters for pipelinestate and gpu handle
	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPipelineState();
	D3D12_GPU_DESCRIPTOR_HANDLE GetFinalGPUHandleForTextures();
private:
	bool finalized;

	// Material properties
	DirectX::XMFLOAT3 colorTint;

	// Texture-related
	DirectX::XMFLOAT2 uvOffset;
	DirectX::XMFLOAT2 uvScale;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
	
	const int numTexSlots = 4;

	//array to hold the four tex types we'll need
	//albedo normal, metalness, roughness in that order
	D3D12_CPU_DESCRIPTOR_HANDLE textureSRVsBySlot[numTexSlots];

	//location of first srv in heap
	D3D12_GPU_DESCRIPTOR_HANDLE finalGPUHandleForSRVs;
};

