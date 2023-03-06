#include "Material.h"

Material::Material(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState, DirectX::XMFLOAT4 tint, MaterialType type, DirectX::XMFLOAT2 uvScale, DirectX::XMFLOAT2 uvOffset)
{
    this->pipelineState = pipelineState;
    colorTint = tint;
    this->type = type;
    this->uvScale = uvScale;
    this->uvOffset = uvOffset;
}

DirectX::XMFLOAT2 Material::GetUVScale()
{
    return uvScale;
}

DirectX::XMFLOAT2 Material::GetUVOffset()
{
    return uvOffset;
}

DirectX::XMFLOAT4 Material::GetColorTint()
{
    return colorTint;
}

MaterialType Material::GetType() {
    return type;
}

void Material::SetUVScale(DirectX::XMFLOAT2 scale)
{
    uvScale = scale;
}

void Material::SetUVOffset(DirectX::XMFLOAT2 offset)
{
    uvOffset = offset;
}

void Material::SetColorTint(DirectX::XMFLOAT4 tint)
{
    colorTint = tint;
}

void Material::SetType(MaterialType type) {
    this->type = type;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> Material::GetPipelineState() 
{
    return pipelineState;
}

D3D12_GPU_DESCRIPTOR_HANDLE Material::GetFinalGPUHandleForTextures()
{
    return finalGPUHandleForSRVs;
}

void Material::AddTexture(D3D12_CPU_DESCRIPTOR_HANDLE srv, int slot)
{
    //exit early if slot doesn't fit into array
    if (slot < 0 || slot >= numTexSlots) {
        return;
    }

    textureSRVsBySlot[slot] = srv;
}

void Material::FinalizeMaterial()
{
    //if already finalized prevent
    //finalizing again
    if (finalized) {
        return;
    }

    DX12Helper* dx12Helper = &DX12Helper::GetInstance();

    for (int i = 0; i < numTexSlots; i++) {
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = dx12Helper->CopySRVsToDescriptorHeapAndGetGPUDescriptorHandle(
            textureSRVsBySlot[i], 1);
        
        //if this is the first one we're copying
        //store the handle so we can reference the 
        //whole area on the gpu
        if (i == 0) {
            finalGPUHandleForSRVs = gpuHandle;
        }
    }

    //set finalize to true so we can't finalize again
    finalized = true;
}
