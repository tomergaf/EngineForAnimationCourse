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

#include "Utility.h"

using namespace cg3d;
using namespace std;

#define MAX_HEALTH 5.0 //TODO make this config file property
#define MOVE_SPEED 10.0 //TODO make this config file property

Snake::Snake(std::shared_ptr<cg3d::Material> program, std::shared_ptr<cg3d::Mesh> mesh, int links){
    snakeMesh = mesh;
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

std::shared_ptr<Snake> Snake::CreateSnake(std::shared_ptr<cg3d::Material> program, std::shared_ptr<cg3d::Mesh> mesh, int numOfLinks){
    return std::shared_ptr<Snake>{new Snake{program, mesh, numOfLinks}};
 }

 void Snake::Die()
 {
    isAlive = false;
    gameManager.GameEnd();
 }

void Snake::InitSnake(){
   
    auto snake{Model::Create("snake", snakeMesh, snakeProgram)};


    auto morphFunc = [](Model* model, cg3d::Visitor* visitor) {
        // static float prevDistance = -1;
        // float distance = (visitor->view * visitor->norm * model->GetAggregatedTransform()).norm();
        // if (prevDistance != distance)
        //     debug(model->name, " distance from camera: ", prevDistance = distance);
        // return distance > 3 ? 1 : 0;
        return 0;
    };

    autoSnake = AutoMorphingModel::Create(*snake, morphFunc);
    autoSnake->showWireframe = false;

    //init links

    // auto material{ std::make_shared<Material>("material", snakeProgram)}; // empty material
    auto cylMesh{IglLoader::MeshFromFiles("cyl_igl","data/xcylinder.obj")};
    float scaleFactor = 0.3f; 
    std::vector<std::shared_ptr<cg3d::Model>> cyls;
    cyls.push_back( Model::Create("cyl",cylMesh, snakeProgram));
    cyls[0]->Scale(scaleFactor,cg3d::Movable::Axis::X);
    // cyls[0]->SetCenter(Eigen::Vector3f(0,0,-0.8f*scaleFactor));
    cyls[0]->SetCenter(Eigen::Vector3f(-0.8f*scaleFactor,0,0));
    // cyls[0]->RotateByDegree(90, Eigen::Vector3f(0,1,0));
    autoSnake->AddChild(cyls[0]);

    for(int i = 1;i < numOfLinks; i++)
        { 
        //cyls
        cyls.push_back( Model::Create("cyl", cylMesh, snakeProgram));
        cyls[i]->Scale(scaleFactor,cg3d::Movable::Axis::X);   
        // cyls[i]->Translate(1.6f*scaleFactor,Axis::Z);
        cyls[i]->Translate(1.6f*scaleFactor,cg3d::Movable::Axis::X);
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
    if(currHealth==0){
        Die();
    }
}

void Snake::IncreaseHealth(float amount)
{
    currHealth = (currHealth + amount)<=maxHealth ? currHealth+amount : maxHealth;
}

void Snake::InitGameValues(){
    isAlive = true;
    shouldAnimate = false;
    maxHealth = MAX_HEALTH;
    currHealth = maxHealth;
    moveSpeed = MOVE_SPEED;
}
