#include "Obstacle.h"
#include <utility>
#include "Utility.h"
#include "SnakeGame.h"
#include "GameManager.h"
#include "GameObject.h"
#include "Util.h"
#include "Snake.h"



#define OBSTACLE_CYCLES 50

namespace Game{

Game::Obstacle::Obstacle(std::shared_ptr<cg3d::Material> material, std::shared_ptr<cg3d::Model> model, SnakeGame* scene) : Game::MovingObject(material, model, scene)
{
    this->cycles = OBSTACLE_CYCLES;
}

void Game::Obstacle::OnCollision()
{
    Util::DebugPrint(name + " " + "collided");
    RunAction();

    
}

void Game::Obstacle::RunAction(){
     // dont hide
    // model->isHidden=true;
    //kill and timeout
    permaGone = false;
    SetTimeOut();
    // update health
    //tell snake its colliding
    scene->gameManager->snake->GetHit(damage);
    
    // notify game manager

}


void Game::Obstacle::Update()
{
    
    if(!AdvanceTime()) //if not time to move or is not active, do not proceed
        return;
    // proceed to check collisions with other objects
    for(auto & elem : scene->gameManager->gameObjects){
        if (elem->name == this->name ) //temp - do not collide with self
            continue;
        if(isActive && CollidingWith(elem))
            OnCollision();
    }
    // Move();
    
    
}

}