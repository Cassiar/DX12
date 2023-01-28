#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <string>

#include "Vertex.h"

#pragma comment(lib, "d3d12.lib")
//#pragma comment(lib, "dxgi.lib")

class Mesh
{
public:
	Mesh(Vertex* vertArray, size_t numVerts, unsigned int* indexArray, size_t numIndices, Microsoft::WRL::ComPtr<ID3D12Device> device);
	Mesh(const std::wstring& objFile, Microsoft::WRL::ComPtr<ID3D12Device> device);
	~Mesh();

	// Getters for mesh data
	Microsoft::WRL::ComPtr<ID3D12Resource> GetVertexBuffer();
	Microsoft::WRL::ComPtr<ID3D12Resource> GetIndexBuffer();
	//return a ptr to the struct
	D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferView();
	//return a ptr to the struct
	D3D12_INDEX_BUFFER_VIEW* GetIndexBufferView();
	unsigned int GetIndexCount();

	// Basic mesh drawing
	void SetBuffersAndDraw();

private:
	// D3D buffers
	Microsoft::WRL::ComPtr<ID3D12Resource> vb;
	Microsoft::WRL::ComPtr<ID3D12Resource> ib;
	
	D3D12_VERTEX_BUFFER_VIEW vbView;
	D3D12_INDEX_BUFFER_VIEW ibView;

	// Total indices in this mesh
	unsigned int numIndices;

	// Helper for creating buffers (in the event we add more constructor overloads)
	void CreateBuffers(Vertex* vertArray, size_t numVerts, unsigned int* indexArray, size_t numIndices, Microsoft::WRL::ComPtr<ID3D12Device> device);
	void CalculateTangents(Vertex* verts, size_t numVerts, unsigned int* indices, size_t numIndices);
};

