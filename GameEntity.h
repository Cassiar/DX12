#pragma once
#include "Transform.h"
#include "Mesh.h"
#include <memory>

class GameEntity
{
public:
	GameEntity(std::shared_ptr<Mesh> mesh);
	~GameEntity();

	//get shared_ptr to mesh
	std::shared_ptr<Mesh> GetMesh();
	Transform* GetTransform();

private:
	std::shared_ptr<Mesh> mesh;
	Transform transform;
};

