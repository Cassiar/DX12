#include "GameEntity.h"

GameEntity::GameEntity(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material) {
    this->mesh = mesh;
    this->material = material;
    transform = Transform();
}

GameEntity::~GameEntity()
{
}

std::shared_ptr<Mesh> GameEntity::GetMesh()
{
    return mesh;
}

Transform* GameEntity::GetTransform() {
    return &transform;
}

std::shared_ptr<Material> GameEntity::GetMaterial()
{
    return material;
}

void GameEntity::SetMaterial(std::shared_ptr<Material> material)
{
    this->material = material;
}
