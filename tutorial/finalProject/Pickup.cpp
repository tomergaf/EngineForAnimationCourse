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


void Game::Pickup::Update()
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