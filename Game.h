#pragma once

#include "DXCore.h"
#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <memory>
#include <vector>

#include "Camera.h"
#include "Mesh.h"
#include "GameEntity.h"
#include "DX12Helper.h"
#include "Material.h"
#include "Lights.h"

class Game 
	: public DXCore
{

public:
	Game(HINSTANCE hInstance);
	~Game();

	// Overridden setup and game loop methods, which
	// will be called automatically
	void Init();
	void OnResize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);

private:

	// Initialization helper methods - feel free to customize, combine, remove, etc.
	void LoadMeshes();

	void LoadTexturesAndCreateMaterials();

	//dx12 helper, replace LoadShaders effectively
	void CreateRootSigAndPipelineState();

	//create entities from meshes, textures, shaders etc
	void CreateEntities();

	//create lights for our pixel shader
	void CreateLights();

	void CreateGui(float deltaTime);

	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the
	//     Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr

	//DX12 buffers and signatures
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;

	//Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
	//Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;

	//D3D12_VERTEX_BUFFER_VIEW vbView;
	//D3D12_INDEX_BUFFER_VIEW ibView;

	DX12Helper* dx12Helper;

	std::shared_ptr<Camera> camera;
	int raysPerPixel = 25;
	int maxRecursion = 10;

	//hold basic shapes for testing
	std::shared_ptr<Mesh> sphereMesh;
	std::shared_ptr<Mesh> helixMesh;
	std::shared_ptr<Mesh> cubeMesh;

	std::vector<std::shared_ptr<Material>> materials;
	std::vector<std::shared_ptr<GameEntity>> entities;

	// Lights
	std::vector<Light> lights;
	int lightCount;
	bool showPointLights;
};

