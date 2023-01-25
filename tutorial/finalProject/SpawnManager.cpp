#include  "SpawnManager.h"
#include  "SnakeGame.h"
#include  "Obstacle.h"
#include  "Pickup.h"
#include  "Model.h"
#include  "HealthPickup.h"
#include  "Util.h"
#include "IglMeshLoader.h"

using namespace cg3d;
namespace Game{

SpawnManager::SpawnManager(float xSize, float ySize, float zSize, SnakeGame* scene){
    this->xSize = xSize;
    this->ySize = ySize;
    this->zSize = zSize;
    this->scene = scene;
    InitAssets();
}

void SpawnManager::InitAssets(){
    modelShader = std::make_shared<Program>("shaders/basicShader"); 
    obstacleMaterial = std::make_shared<Material>("bricks", modelShader);
    healthMaterial = std::make_shared<Material>("grass", modelShader);
    pickupMaterial = std::make_shared<Material>("pickup", modelShader);
    
    obstacleMaterial->AddTexture(0, "textures/bricks.jpg", 2);
    healthMaterial->AddTexture(0, "textures/grass.bmp", 2);
    pickupMaterial->AddTexture(0, "textures/pal.png", 2);
    
    obstacleMesh = IglLoader::MeshFromFiles("cubeMesh", "data/cube.off");
    pickupMesh = IglLoader::MeshFromFiles("sphereMesh", "data/sphere.obj");
    healthMesh = IglLoader::MeshFromFiles("cylMesh", "data/xcylinder.obj");
}

void SpawnManager::SpawnWave(int pickups, int obstacles, int health)
{
    for(int i = 0; i<pickups; i++){
        SpawnPickupRandom(PICKUP);
    }
    for(int i = 0; i<obstacles; i++){
        //change to spawnObstacle
        SpawnPickupRandom(OBSTACLE);
    }
    for(int i = 0; i<health; i++){
        //change to spanwHealth
        SpawnPickupRandom(HEALTHPICKUP);
    }
}

std::shared_ptr<cg3d::Model> SpawnManager::CreatePickupModel()
{
    return Model::Create("Pickup", pickupMesh, pickupMaterial);
}

std::shared_ptr<cg3d::Model> SpawnManager::CreateObstacleModel()
{
    return Model::Create("Obstacle", obstacleMesh, obstacleMaterial);
}

std::shared_ptr<cg3d::Model> SpawnManager::CreateHealthPickupModel()
{
    return Model::Create("HealthPickup", healthMesh, healthMaterial);
}

void SpawnManager::SpawnPickupRandom(InteractableObjects type){
    float randX = Util::GenerateRandomInRange(-xSize, xSize);
    float randY = Util::GenerateRandomInRange(-ySize, ySize);
    float randZ = Util::GenerateRandomInRange(-zSize, zSize);
    SpawnPickup(randX, randY, randZ, type);
}

void SpawnManager::SpawnPickup(float x, float y, float z, InteractableObjects type){
    //create a model
    std::shared_ptr<cg3d::Model> model;
    std::shared_ptr<MovingObject> pickup;

    // spawn the object
    switch(type){
        case PICKUP:
        //TODO - update manager
        model = CreatePickupModel();
        pickup = std::make_shared<Pickup>(*Pickup::SpawnObject(x, y, z, pickupMaterial, model, scene));
        break;

        case OBSTACLE:
        model = CreateObstacleModel();
        pickup = std::make_shared<Obstacle>(*Obstacle::SpawnObject(x, y, z, pickupMaterial, model, scene));
        break;

        case HEALTHPICKUP:
        model = CreateHealthPickupModel();
        pickup = std::make_shared<HealthPickup>(*HealthPickup::SpawnObject(x, y, z, pickupMaterial, model, scene));
        break;
    }
    scene->AddInteractable(pickup);

    // update the manager
}


}