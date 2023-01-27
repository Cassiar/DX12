#include "GameEntity.h"

GameEntity::GameEntity(std::shared_ptr<Mesh> mesh) {
    this->mesh = mesh;
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