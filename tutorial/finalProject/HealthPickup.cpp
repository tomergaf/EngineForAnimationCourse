#include "HealthPickup.h"
#include "GameManager.h"
#include "SnakeGame.h"
#include "Snake.h"
#include "Pickup.h"

#define HEALTH_CYCLES 50

namespace Game{

Game::HealthPickup::HealthPickup(std::shared_ptr<cg3d::Material> material, std::shared_ptr<cg3d::Model> model, SnakeGame* scene) : Game::Pickup(material, model, scene)
{
    this->cycles = HEALTH_CYCLES;
}


void Game::HealthPickup::RunAction(){
    
    // hide
    model->isHidden=true;
    //kill and timeout
    permaGone = true;
     SetTimeOut();
    // update health
    scene->gameManager->snake->IncreaseHealth(healthValue);
    // notify game manager

}
}

