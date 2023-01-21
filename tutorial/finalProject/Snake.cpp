#include "Snake.h"
#include <memory>
#include <per_face_normals.h>
#include <read_triangle_mesh.h>
#include <utility>
#include <vector>
#include "GLFW/glfw3.h"
#include "Mesh.h"
#include "PickVisitor.h"
#include "Renderer.h"
#include "ObjLoader.h"
#include "Movable.h"
#include "AutoMorphingModel.h"
#include "IglMeshLoader.h"

#include "igl/per_vertex_normals.h"
#include "igl/per_face_normals.h"
#include "igl/unproject_onto_mesh.h"
#include "igl/edge_flaps.h"
#include "igl/loop.h"
#include "igl/upsample.h"
#include "igl/AABB.h"
#include "igl/parallel_for.h"
#include "igl/shortest_edge_and_midpoint.h"
#include "igl/circulation.h"
#include "igl/edge_midpoints.h"
#include "igl/collapse_edge.h"
#include "igl/edge_collapse_is_valid.h"
#include "igl/write_triangle_mesh.h"
#include "GameObject.h"

#include "Utility.h"
#include "Util.h"

using namespace cg3d;
using namespace std;

#define MAX_HEALTH 5.0 //TODO make this config file property
#define MOVE_SPEED 0.005 //TODO make this config file property

namespace Game{


Snake::Snake(std::shared_ptr<cg3d::Material> program, std::shared_ptr<cg3d::AutoMorphingModel> model, int links, SnakeGame* scene): autoSnake(model) {
    // snakeMesh = mesh;
    this->scene = scene;
    this->gameManager = scene->gameManager;
    snakeProgram = program;
    numOfLinks = links;
    InitSnake();
}

int Snake::GetNumOfLinks(){
    return numOfLinks;
}

std::shared_ptr<AutoMorphingModel> Snake::GetModel(){
    return autoSnake;
 }

std::shared_ptr<Snake> Snake::CreateSnake(std::shared_ptr<cg3d::Material> program, std::shared_ptr<cg3d::AutoMorphingModel> model, int numOfLinks, SnakeGame* scene){
    // return std::shared_ptr<Snake>{new Snake{program, model, numOfLinks}};
    return std::shared_ptr<Snake>{new Snake(program, model, numOfLinks, scene)};
 }

 void Snake::GetHit(float amount)
 {
    //multiply by modifier - TEMP no modifier
    DecreaseHealth(amount);
 }

 void Snake::Die()
 {
    isAlive = false;
    Util::DebugPrint("snake dead");
    gameManager->GameEnd();
 }

 void Snake::AddVelocity(float amount, cg3d::Model::Axis axis)
 {
    //handle x velocity
    if (axis == cg3d::Model::Axis::X || axis == cg3d::Model::Axis::XY || axis == cg3d::Model::Axis::XZ || axis == cg3d::Model::Axis::XYZ) 
        velocityX += amount;
    //handle y velocity
    if (axis == cg3d::Model::Axis::Y || axis == cg3d::Model::Axis::XY || axis == cg3d::Model::Axis::YZ || axis == cg3d::Model::Axis::XYZ) 
        velocityY += amount;
    //handle z velocity
    if (axis == cg3d::Model::Axis::Z || axis == cg3d::Model::Axis::XZ || axis == cg3d::Model::Axis::YZ || axis == cg3d::Model::Axis::XYZ) 
        velocityZ += amount;
 }

 void Snake::StopMoving()
 {
    velocityX = velocityY = velocityZ = 0;
 }

 float Snake::GetMoveSpeed()
 {
    return moveSpeed;
 }

 bool Snake::IsColliding()
 {
    return false;
 }

 Eigen::Vector3f Snake::GetMoveDirection()
 {
    return Eigen::Vector3f(velocityX,velocityY,velocityZ).normalized();
 }

void Snake::InitSnake(){
   
    //add head to list
    GameObject* link;
    link = new GameObject(snakeProgram, autoSnake, scene);
    links.push_back(std::make_shared<Game::GameObject>(*link));

    //create links
    auto cylMesh{IglLoader::MeshFromFiles("cyl_igl","data/xcylinder.obj")};
    float scaleFactor = 0.3f;
    // float scaleFactor = 1;
    std::vector<std::shared_ptr<cg3d::Model>> cyls;
    cyls.push_back( Model::Create("cyl",cylMesh, snakeProgram));
    cyls[0]->Scale(scaleFactor,cg3d::Movable::Axis::X);
    // cyls[0]->SetCenter(Eigen::Vector3f(0,0,-0.8f*scaleFactor));
    link = new GameObject(snakeProgram, cyls[0], scene);
    links.push_back(std::make_shared<Game::GameObject>(*link));
    cyls[0]->SetCenter(Eigen::Vector3f(-0.8f*scaleFactor,0,0));
    // cyls[0]->RotateByDegree(90, Eigen::Vector3f(0,1,0));
    autoSnake->AddChild(cyls[0]);

    // add first link to game objects list
    //TEMP

    for(int i = 1;i < numOfLinks; i++)
        { 
        //cyls
        cyls.push_back( Model::Create("cyl", cylMesh, snakeProgram));
        cyls[i]->Scale(scaleFactor,cg3d::Movable::Axis::X);   
        // cyls[i]->Translate(1.6f*scaleFactor,Axis::Z);
        cyls[i]->Translate(1.6f*scaleFactor,cg3d::Movable::Axis::X);
        link = new GameObject(snakeProgram, cyls[i], scene);
        links.push_back(std::make_shared<Game::GameObject>(*link));
        //TEMP

        cyls[i]->SetCenter(Eigen::Vector3f(-0.8f*scaleFactor,0,0));
        // cyls[i]->SetCenter(Eigen::Vector3f(0,0,-0.8f*scaleFactor));
        cyls[i-1]->AddChild(cyls[i]);   
      
    }
    // cyls[0]->Translate({0,0,0.8f*scaleFactor});
    cyls[0]->Translate({0.8f*scaleFactor,0,0});

    InitGameValues();
    
}

void Snake::DecreaseHealth(float amount)
{
    currHealth = (currHealth - amount)>=0 ? currHealth-amount : 0;
    Util::DebugPrint("snake health: " + std::to_string(currHealth));
    if(currHealth==0){
        Die();
    }
}

void Snake::IncreaseHealth(float amount)
{
    currHealth = (currHealth + amount)<=maxHealth ? currHealth+amount : maxHealth;
    Util::DebugPrint("snake health: " + std::to_string(currHealth));
}

void Snake::InitGameValues(){
    isAlive = true;
    shouldAnimate = false;
    maxHealth = MAX_HEALTH;
    currHealth = maxHealth;
    moveSpeed = MOVE_SPEED;
    StopMoving();
}
}