#pragma once
#include "Transform.h"
#include "Mesh.h"
#include "Material.h"

#include <memory>

class GameEntity
{
public:
	GameEntity(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material);
	~GameEntity();

	//get shared_ptr to mesh
	std::shared_ptr<Mesh> GetMesh();
	Transform* GetTransform();

	std::shared_ptr<Material> GetMaterial();
	void SetMaterial(std::shared_ptr<Material> material);
private:
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> material;
	Transform transform;
};

