#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "Helpers.h"
#include "BufferStructs.h"
#include "DX12Helper.h"
#include "RaytracingHelper.h"

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;
using namespace std;

// Helper macro for getting a float between min and max
//from Chris's code
#define RandomRange(min, max) (float)rand() / RAND_MAX * (max - min) + min

#define NUM_SPHERES 20

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// Direct3D itself, and our window, are not ready at this point!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,			// The application's handle
		L"DirectX Game",	// Text for the window's title bar (as a wide-character string)
		1280,				// Width of the window's client area
		720,				// Height of the window's client area
		false,				// Sync the framerate to the monitor refresh? (lock framerate)
		true)				// Show extra stats (fps) in title bar?
{
#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
#endif
	
	//ibView = {};
	//vbView = {};

	camera = make_shared<Camera>(
		0.0f, 0.0f, -15.0f,	// Position
		5.0f,				// Move speed (world units)
		0.002f,				// Look speed (cursor movement pixels --> radians for rotation)
		XM_PIDIV4,			// Field of view
		(float)windowWidth / windowHeight,  // Aspect ratio
		0.01f,				// Near clip
		100.0f,				// Far clip
		CameraProjectionType::Perspective);

	dx12Helper = &DX12Helper::GetInstance();
}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Delete all objects manually created within this class
//  - Release() all Direct3D objects created within this class
// --------------------------------------------------------
Game::~Game()
{
	// Call delete or delete[] on any objects or arrays you've
	// created using new or new[] within this class
	// - Note: this is unnecessary if using smart pointers

	// Call Release() on any Direct3D objects made within this class
	// - Note: this is unnecessary for D3D objects stored in ComPtrs
	DX12Helper::GetInstance().WaitForGPU();

	//RaytracingHelper::GetINstance().

	//delete dx12Helper;
	delete& RaytracingHelper::GetInstance();
}

// --------------------------------------------------------
// Called once per program, after Direct3D and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{

	//init DXR
	RaytracingHelper::GetInstance().Initialize(
		windowWidth,
		windowHeight,
		device,
		commandQueue,
		commandList,
		FixPath(L"Raytracing.cso"));

	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
	CreateRootSigAndPipelineState();
	LoadMeshes();
	LoadTexturesAndCreateMaterials();
	CreateEntities();
	CreateLights();
}

// --------------------------------------------------------
// Loads two basic shaders, then creates the root sig
// and pipeline state object for our basic demo
// --------------------------------------------------------
void Game::CreateRootSigAndPipelineState()
{
	//BLOBs (Binary Large OBjects) to hold raw shader cyte code
	Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderByteCode;
	Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderByteCode;

	// Load shaders
	{
		// Read our compiled vertex shader code into a blob
		// - Essentially just "open the file and plop its contents here"
		D3DReadFileToBlob(FixPath(L"VertexShader.cso").c_str(),
			vertexShaderByteCode.GetAddressOf());
		D3DReadFileToBlob(FixPath(L"PixelShader.cso").c_str(),
			pixelShaderByteCode.GetAddressOf());
	}
	// Input layout

	// Create an input layout that describes the vertex format
	// used by the vertex shader we're using
	// - This is used by the pipeline to know how to interpret the raw data
	// sitting inside a vertex buffer
	// Set up the first element - a position, which is 3 float values
	const unsigned int inputElementCount = 4;
	D3D12_INPUT_ELEMENT_DESC inputElements[inputElementCount] = {};
	{
		inputElements[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT; // R32 G32 B32 = float3
		inputElements[0].SemanticName = "POSITION"; // Name must match semantic in shader
		inputElements[0].SemanticIndex = 0; // This is the first POSITION semantic

		inputElements[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[1].Format = DXGI_FORMAT_R32G32_FLOAT; // R32 G32 = float2
		inputElements[1].SemanticName = "TEXCOORD";
		inputElements[1].SemanticIndex = 0; // This is the first TEXCOORD semantic
		
		inputElements[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[2].Format = DXGI_FORMAT_R32G32B32_FLOAT; // R32 G32 B32 = float3
		inputElements[2].SemanticName = "NORMAL";
		inputElements[2].SemanticIndex = 0; // This is the first NORMAL semantic
		
		inputElements[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[3].Format = DXGI_FORMAT_R32G32B32_FLOAT; // R32 G32 B32 = float3
		inputElements[3].SemanticName = "TANGENT";
		inputElements[3].SemanticIndex = 0; // This is the first TANGENT semantic
	}

	// Root Signature
	{
		// Describe the range of CBVs needed for the vertex shader
		D3D12_DESCRIPTOR_RANGE cbvRangeVS = {};
		cbvRangeVS.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvRangeVS.NumDescriptors = 1;
		cbvRangeVS.BaseShaderRegister = 0;
		cbvRangeVS.RegisterSpace = 0;
		cbvRangeVS.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		// Describe the range of CBVs needed for the pixel shader
		D3D12_DESCRIPTOR_RANGE cbvRangePS = {};
		cbvRangePS.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvRangePS.NumDescriptors = 1;
		cbvRangePS.BaseShaderRegister = 0;
		cbvRangePS.RegisterSpace = 0;
		cbvRangePS.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		// Create a range of SRV's for textures
		D3D12_DESCRIPTOR_RANGE srvRange = {};
		srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvRange.NumDescriptors = 4; // Set to max number of textures at once (match pixel shader!)
		srvRange.BaseShaderRegister = 0; // Starts at s0 (match pixel shader!)
		srvRange.RegisterSpace = 0;
		srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		// Create the root parameters
		D3D12_ROOT_PARAMETER rootParams[3] = {};
		// CBV table param for vertex shader
		rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[0].DescriptorTable.pDescriptorRanges = &cbvRangeVS;
		// CBV table param for pixel shader
		rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[1].DescriptorTable.pDescriptorRanges = &cbvRangePS;
		// SRV table param
		rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[2].DescriptorTable.pDescriptorRanges = &srvRange;
		// Create a single static sampler (available to all pixel shaders at the same slot)
		D3D12_STATIC_SAMPLER_DESC anisoWrap = {};
		anisoWrap.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		anisoWrap.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		anisoWrap.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		anisoWrap.Filter = D3D12_FILTER_ANISOTROPIC;
		anisoWrap.MaxAnisotropy = 16;
		anisoWrap.MaxLOD = D3D12_FLOAT32_MAX;
		anisoWrap.ShaderRegister = 0; // register(s0)
		anisoWrap.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		D3D12_STATIC_SAMPLER_DESC samplers[] = { anisoWrap };
		// Describe and serialize the root signature
		D3D12_ROOT_SIGNATURE_DESC rootSig = {};
		rootSig.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		rootSig.NumParameters = ARRAYSIZE(rootParams);
		rootSig.pParameters = rootParams;
		rootSig.NumStaticSamplers = ARRAYSIZE(samplers);
		rootSig.pStaticSamplers = samplers;

		ID3DBlob* serializedRootSig = 0;
		ID3DBlob* errors = 0;
		D3D12SerializeRootSignature(
			&rootSig,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&serializedRootSig,
			&errors);
		// Check for errors during serialization
		if (errors != 0)
		{
			OutputDebugString((wchar_t*)errors->GetBufferPointer());
		}
		// Actually create the root sig
		device->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(rootSignature.GetAddressOf()));
	}

	// Pipeline state
	{
		// Describe the pipeline state
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		// -- Input assembler related ---
		psoDesc.InputLayout.NumElements = inputElementCount;
		psoDesc.InputLayout.pInputElementDescs = inputElements;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		// Root sig
		psoDesc.pRootSignature = rootSignature.Get();
		// -- Shaders (VS/PS) ---
		psoDesc.VS.pShaderBytecode = vertexShaderByteCode->GetBufferPointer();
		psoDesc.VS.BytecodeLength = vertexShaderByteCode->GetBufferSize();
		psoDesc.PS.pShaderBytecode = pixelShaderByteCode->GetBufferPointer();
		psoDesc.PS.BytecodeLength = pixelShaderByteCode->GetBufferSize();
		// -- Render targets ---
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;
		// -- States ---
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		psoDesc.RasterizerState.DepthClipEnable = true;
		psoDesc.DepthStencilState.DepthEnable = true;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
		psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask =
			D3D12_COLOR_WRITE_ENABLE_ALL;
		// -- Misc ---
		psoDesc.SampleMask = 0xffffffff;
		// Create the pipe state object
		device->CreateGraphicsPipelineState(&psoDesc,
			IID_PPV_ARGS(pipelineState.GetAddressOf()));
	}
}



// --------------------------------------------------------
// Creates the geometry we're going to draw - a single triangle for now
// --------------------------------------------------------
void Game::LoadMeshes()
{
	// Make the meshes
	sphereMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/sphere.obj").c_str(), device);
	helixMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/helix.obj").c_str(), device);
	cubeMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/cube.obj").c_str(), device);
}

void Game::LoadTexturesAndCreateMaterials()
{
	// Create some temporary variables to represent colors
	// - Not necessary, just makes things more readable
	XMFLOAT4 red = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f);
	XMFLOAT4 green = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	XMFLOAT4 blue = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	XMFLOAT4 grey = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);

	//bronze material
	materials.push_back(std::make_shared<Material>(pipelineState, red, MaterialType::Transparent));
	//cobblestone
	materials.push_back(std::make_shared<Material>(pipelineState, green));
	//paint
	materials.push_back(std::make_shared<Material>(pipelineState, blue));
	//scratched
	materials.push_back(std::make_shared<Material>(pipelineState, grey));

	//make a bunch of materials with random colors
	for (int i = 0; i < NUM_SPHERES; i++) {
		XMFLOAT4 randColor = XMFLOAT4(RandomRange(0, 1.0f), RandomRange(0, 1.0f), RandomRange(0, 1.0f), RandomRange(0,1.0f));
		//random chance to be emissive or dielectric (glass)
		MaterialType type = MaterialType::Normal;
		int rng = RandomRange(0, 1.0f);
		if (i%3 == 1) {
			type = MaterialType::Emissive;
			randColor.w = RandomRange(1.0f, 2.0f);//increase w to act as intensity
		}
		//else if (i%3==2) {
		//	type = MaterialType::Transparent;
		//}
		materials.push_back(std::make_shared<Material>(pipelineState, randColor, type));
	}

	//add appropriate textures to each material
	//materials[0]->AddTexture(dx12Helper->LoadTexture(FixPath(L"../../Assets/Textures/bronze_albedo.png").c_str()), 0);
	//materials[0]->AddTexture(dx12Helper->LoadTexture(FixPath(L"../../Assets/Textures/bronze_metal.png").c_str()), 1);
	//materials[0]->AddTexture(dx12Helper->LoadTexture(FixPath(L"../../Assets/Textures/bronze_normals.png").c_str()), 2);
	//materials[0]->AddTexture(dx12Helper->LoadTexture(FixPath(L"../../Assets/Textures/bronze_roughness.png").c_str()), 3);
	//
	//materials[1]->AddTexture(dx12Helper->LoadTexture(FixPath(L"../../Assets/Textures/cobblestone_albedo.png").c_str()), 0);
	//materials[1]->AddTexture(dx12Helper->LoadTexture(FixPath(L"../../Assets/Textures/cobblestone_metal.png").c_str()), 1);
	//materials[1]->AddTexture(dx12Helper->LoadTexture(FixPath(L"../../Assets/Textures/cobblestone_normals.png").c_str()), 2);
	//materials[1]->AddTexture(dx12Helper->LoadTexture(FixPath(L"../../Assets/Textures/cobblestone_roughness.png").c_str()), 3);
	//
	//materials[2]->AddTexture(dx12Helper->LoadTexture(FixPath(L"../../Assets/Textures/paint_albedo.png").c_str()), 0);
	//materials[2]->AddTexture(dx12Helper->LoadTexture(FixPath(L"../../Assets/Textures/paint_metal.png").c_str()), 1);
	//materials[2]->AddTexture(dx12Helper->LoadTexture(FixPath(L"../../Assets/Textures/paint_normals.png").c_str()), 2);
	//materials[2]->AddTexture(dx12Helper->LoadTexture(FixPath(L"../../Assets/Textures/paint_roughness.png").c_str()), 3);
	//
	//materials[3]->AddTexture(dx12Helper->LoadTexture(FixPath(L"../../Assets/Textures/scratched_albedo.png").c_str()), 0);
	//materials[3]->AddTexture(dx12Helper->LoadTexture(FixPath(L"../../Assets/Textures/scratched_metal.png").c_str()), 1);
	//materials[3]->AddTexture(dx12Helper->LoadTexture(FixPath(L"../../Assets/Textures/scratched_normals.png").c_str()), 2);
	//materials[3]->AddTexture(dx12Helper->LoadTexture(FixPath(L"../../Assets/Textures/scratched_roughness.png").c_str()), 3);

	//
	//for (int i = 0; i < materials.size(); i++) {

	//	//'fake' textures to prevent seg faults
	//	materials[i]->AddTexture(dx12Helper->LoadTexture(FixPath(L"../../Assets/Textures/scratched_albedo.png").c_str()), 0);
	//	materials[i]->AddTexture(dx12Helper->LoadTexture(FixPath(L"../../Assets/Textures/scratched_metal.png").c_str()), 1);
	//	materials[i]->AddTexture(dx12Helper->LoadTexture(FixPath(L"../../Assets/Textures/scratched_normals.png").c_str()), 2);
	//	materials[i]->AddTexture(dx12Helper->LoadTexture(FixPath(L"../../Assets/Textures/scratched_roughness.png").c_str()), 3);

	//	materials[i]->FinalizeMaterial();
	//}
}

void Game::CreateEntities() 
{
	entities.push_back(std::make_shared<GameEntity>(sphereMesh, materials[0]));
	entities.push_back(std::make_shared<GameEntity>(helixMesh, materials[1]));
	entities.push_back(std::make_shared<GameEntity>(cubeMesh, materials[2]));
	entities.push_back(std::make_shared<GameEntity>(cubeMesh, materials[3]));
	//ground should be grey
	entities.push_back(std::make_shared<GameEntity>(cubeMesh, materials[3]));

	//add a bunch of random sphere
	for (int i = 0; i < NUM_SPHERES; i++) {
		//4 basic colors for red, green, blue, and white
		int index = RandomRange(4, NUM_SPHERES + 4);
		entities.push_back(std::make_shared<GameEntity>(sphereMesh, materials[index]));
	}

	//adjust transform to not be overlapping
	entities[0]->GetTransform()->SetPosition(0, 2.5f, 0);
	entities[1]->GetTransform()->SetPosition(5, 0, 0);
	entities[2]->GetTransform()->SetPosition(0, 0, -5);
	entities[3]->GetTransform()->SetPosition(0, 0, 5);
	//make a big square to be the ground
	entities[4]->GetTransform()->SetPosition(0, -13, 0);
	entities[4]->GetTransform()->SetScale(25, 25, 25);

	//move the spheres to random positions
	for (int i = 5; i < entities.size(); i++) {
		entities[i]->GetTransform()->SetPosition(RandomRange(-5, 5), 0, RandomRange(-5, 5));
		entities[i]->GetTransform()->SetScale(RandomRange(0.1f, 0.5f));
	}

	RaytracingHelper::GetInstance().CreateTopLevelAccelerationStructureForScene(entities);
}

// Code borrowed from Chris Cascioli's Adv. DX11 code
void Game::CreateLights()
{
	// Reset
	lights.clear();

	// Setup directional lights
	Light dir1 = {};
	dir1.Type = LIGHT_TYPE_DIRECTIONAL;
	dir1.Direction = XMFLOAT3(-1, 0, 0);
	dir1.Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	dir1.Intensity = 1.0f;

	Light dir2 = {};
	dir2.Type = LIGHT_TYPE_DIRECTIONAL;
	dir2.Direction = XMFLOAT3(-1, -0.25f, 0);
	dir2.Color = XMFLOAT3(0.2f, 0.2f, 0.2f);
	dir2.Intensity = 1.0f;

	Light dir3 = {};
	dir3.Type = LIGHT_TYPE_DIRECTIONAL;
	dir3.Direction = XMFLOAT3(0, -1, 1);
	dir3.Color = XMFLOAT3(0.2f, 0.2f, 0.2f);
	dir3.Intensity = 1.0f;

	// Add light to the list
	lights.push_back(dir1);
	lights.push_back(dir2);
	lights.push_back(dir3);

	// Create the rest of the lights
	while (lights.size() < 17) //only have 20 lights total
	{
		Light point = {};
		point.Type = LIGHT_TYPE_POINT;
		point.Position = XMFLOAT3(RandomRange(-10.0f, 10.0f), RandomRange(-5.0f, 5.0f), RandomRange(-10.0f, 10.0f));
		point.Color = XMFLOAT3(RandomRange(0, 1), RandomRange(0, 1), RandomRange(0, 1));
		point.Range = RandomRange(5.0f, 10.0f);
		point.Intensity = RandomRange(0.1f, 3.0f);

		// Add to the list
		lights.push_back(point);
	}
}

// --------------------------------------------------------
// Handle resizing to match the new window size.
//  - DXCore needs to resize the back buffer
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
	// Handle base-level DX resize stuff
	DXCore::OnResize();

	RaytracingHelper::GetInstance().ResizeOutputUAV(windowWidth, windowHeight);
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// Example input checking: Quit if the escape key is pressed
	if (Input::GetInstance().KeyDown(VK_ESCAPE)) {
		Quit();
	}

	//entities[0]->GetTransform()->MoveRelative(0, (float)cos(totalTime) / 4, 0);
	entities[0]->GetTransform()->Rotate(deltaTime, deltaTime, 0);
	entities[1]->GetTransform()->Rotate(0, deltaTime, 0);
	entities[2]->GetTransform()->Rotate(0, 0, deltaTime);
	entities[3]->GetTransform()->Rotate(deltaTime, 0, 0);
	//entities[3]->GetTransform()->MoveRelative((float)sin(totalTime) / 4, 0, 0);

	for (int i = 5; i < entities.size(); i++) {

		entities[i]->GetTransform()->MoveRelative(0.01f * sin(totalTime + i), 0, 0.01f * cos(totalTime + i));
	}

	camera->Update(deltaTime);
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Grab the current back buffer for this frame
	//Microsoft::WRL::ComPtr<ID3D12Resource> currentBackBuffer = backBuffers[currentSwapBuffer];
	
	RaytracingHelper::GetInstance().CreateTopLevelAccelerationStructureForScene(entities);

	RaytracingHelper::GetInstance().Raytrace(
		camera, backBuffers[currentSwapBuffer], raysPerPixel, maxRecursion
	);
	
	//=============================
	//comments bellow were old drawing
	//currently handled by raytracinghelper
	//thanks chris!
	//==============================

	// Clearing the render target
	//{
	//	// Transition the back buffer from present to render target
	//	D3D12_RESOURCE_BARRIER rb = {};
	//	rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	//	rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	//	rb.Transition.pResource = currentBackBuffer.Get();
	//	rb.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	//	rb.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	//	rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	//	commandList->ResourceBarrier(1, &rb);
	//	// Background color (black in this case) for clearing
	//	float color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	//	// Clear the RTV
	//	commandList->ClearRenderTargetView(
	//		rtvHandles[currentSwapBuffer],
	//		color,
	//		0, 0); // No scissor rectangles
	//	// Clear the depth buffer, too
	//	commandList->ClearDepthStencilView(
	//		dsvHandle,
	//		D3D12_CLEAR_FLAG_DEPTH,
	//		1.0f, // Max depth = 1.0f
	//		0, // Not clearing stencil, but need a value
	//		0, 0); // No scissor rects
	//}
	//
	//// Rendering here!
	//{
	//	// Set overall pipeline state
	//	commandList->SetPipelineState(pipelineState.Get());
	//	// Root sig (must happen before root descriptor table)
	//	commandList->SetGraphicsRootSignature(rootSignature.Get());
	//	// Set up other commands for rendering
	//	commandList->OMSetRenderTargets(1, &rtvHandles[currentSwapBuffer], true, &dsvHandle);
	//	commandList->RSSetViewports(1, &viewport);
	//	commandList->RSSetScissorRects(1, &scissorRect);

	//	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap =
	//		dx12Helper->GetCBVSRVDescriptorHeap();
	//	commandList->SetDescriptorHeaps(1, descriptorHeap.GetAddressOf());

	//	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//	//draw each object in entity list
	//	for (int i = 0; i < entities.size(); i++) {

	//		//grab material and pass to shader
	//		std::shared_ptr<Material> mat = entities[i]->GetMaterial();
	//		commandList->SetPipelineState(mat->GetPipelineState().Get());

	//		// Set the SRV descriptor handle for this material's textures
	//		// Note: This assumes that descriptor table 2 is for textures (as per our root sig)
	//		commandList->SetGraphicsRootDescriptorTable(2, mat->GetFinalGPUHandleForTextures());

	//		{
	//			VertexShaderExternalData vsData = {};
	//			vsData.world = entities[i]->GetTransform()->GetWorldMatrix();
	//			vsData.worldInverseTranspose = entities[i]->GetTransform()->GetWorldInverseTransposeMatrix();
	//			vsData.view = camera->GetView();
	//			vsData.proj = camera->GetProjection();

	//			D3D12_GPU_DESCRIPTOR_HANDLE cbHandleVS = dx12Helper->FillNextConstantBufferAndGetGPUDescriptorHandle(&vsData, sizeof(VertexShaderExternalData));
	//			commandList->SetGraphicsRootDescriptorTable(0, cbHandleVS);
	//		}

	//		// Pixel shader data and cbuffer setup
	//		{
	//			PixelShaderExternalData psData = {};
	//			psData.uvScale = mat->GetUVScale();
	//			psData.uvOffset = mat->GetUVOffset();
	//			psData.cameraPosition = camera->GetTransform()->GetPosition();
	//			psData.colorTint = mat->GetColorTint();
	//			psData.lightCount = lights.size();
	//			memcpy(psData.lights, &lights[0], sizeof(Light) * MAX_LIGHTS);
	//			// Send this to a chunk of the constant buffer heap
	//			// and grab the GPU handle for it so we can set it for this draw
	//			D3D12_GPU_DESCRIPTOR_HANDLE cbHandlePS =
	//				dx12Helper->FillNextConstantBufferAndGetGPUDescriptorHandle(
	//					(void*)(&psData), sizeof(PixelShaderExternalData));
	//			// Set this constant buffer handle
	//			// Note: This assumes that descriptor table 1 is the
	//			// place to put this particular descriptor. This
	//			// is based on how we set up our root signature.
	//			commandList->SetGraphicsRootDescriptorTable(1, cbHandlePS);
	//		}

	//		D3D12_VERTEX_BUFFER_VIEW tempVBView = entities[i]->GetMesh()->GetVBView();
	//		commandList->IASetVertexBuffers(0, 1, &tempVBView);

	//		D3D12_INDEX_BUFFER_VIEW tempIBView = entities[i]->GetMesh()->GetIBView();
	//		commandList->IASetIndexBuffer(&tempIBView);
	//		commandList->DrawIndexedInstanced(entities[i]->GetMesh()->GetIndexCount(), 1, 0, 0, 0);
	//	}

	//}
	
	// Present
	{
		// Transition back to present
		//D3D12_RESOURCE_BARRIER rb = {};
		//rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		//rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		//rb.Transition.pResource = currentBackBuffer.Get();
		//rb.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		//rb.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		//rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		//commandList->ResourceBarrier(1, &rb);
		
		// Must occur BEFORE present
		//DX12Helper::GetInstance().CloseExecuteAndResetCommandList();
		DX12Helper::GetInstance().WaitForGPU();
		commandAllocator->Reset();
		commandList->Reset(commandAllocator.Get(), 0);
		
		// Present the current back buffer
		bool vsyncNecessary = vsync || !deviceSupportsTearing || isFullscreen;
		swapChain->Present(
			vsyncNecessary ? 1 : 0,
			vsyncNecessary ? 0 : DXGI_PRESENT_ALLOW_TEARING);
		// Figure out which buffer is next
		currentSwapBuffer++;
		if (currentSwapBuffer >= numBackBuffers)
			currentSwapBuffer = 0;
	}
}