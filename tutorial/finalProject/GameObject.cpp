#include "GameObject.h"

namespace Game{

GameObject::GameObject(std::shared_ptr<cg3d::Material> material, std::shared_ptr<cg3d::Model> model)
{
    this->material = material;
    this->model = model;
}

GameObject::GameObject()
{
}
}