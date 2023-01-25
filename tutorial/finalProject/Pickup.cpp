#include "Pickup.h"
#include <utility>
#include "Utility.h"
#include "SnakeGame.h"
#include "GameManager.h"
#include "GameObject.h"
#include "Util.h"



#define PICKUP_CYCLES 50

namespace Game{

Game::Pickup::Pickup(std::shared_ptr<cg3d::Material> material, std::shared_ptr<cg3d::Model> model, SnakeGame* scene) : Game::MovingObject(material, model, scene)
{
    this->cycles = PICKUP_CYCLES;
}

void Game::Pickup::OnCollision()
{
    Util::DebugPrint(name + " " + "collided");
    RunAction();

    
}

void Game::Pickup::RunAction(){
     // hide
    model->isHidden=true;
    //kill and timeout
    permaGone = true;
    SetTimeOut();
    // update score
    scene->gameManager->IncreaseScore(score);
    // notify game manager

}

Pickup* Game::Pickup::SpawnObject(float xCoordinate, float yCoordinate, float zCoordinate, std::shared_ptr<cg3d::Material> material, std::shared_ptr<cg3d::Model> model, SnakeGame* scene){
	Game::Pickup* movingObject = new Game::Pickup{material, model, scene};
	//move to location
	scene->root->AddChild(movingObject->model);
	Eigen::Vector3f posVec{xCoordinate, yCoordinate, zCoordinate};
	movingObject->model->Translate(posVec);
	//add to logs
	Util::DebugPrint(movingObject->name + " added at : " + std::to_string(xCoordinate) + ", " + std::to_string(yCoordinate) + ", " + std::to_string(zCoordinate));
	return movingObject;
	
}


void Game::Pickup::Update()
{
    
    if(!AdvanceTime()) //if not time to move or is not active, do not proceed
        return;
    // proceed to check collisions with other objects
    for(auto & elem : scene->gameManager->gameObjects){
        if (elem->name == this->name ) //temp - do not collide with self
            continue;
        if(isActive && CollidingWith(elem))
            if(elem->partOfSnake){
                OnCollision();
            }
    }
    // Move();
    
    
}

}