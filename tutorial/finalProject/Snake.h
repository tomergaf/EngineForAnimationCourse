#pragma once

#include <Eigen/Geometry>
#include <utility>
#include "Movable.h"
#include "AutoMorphingModel.h"
#include <vector>
#include <string>
#include <cstdint>

#include "GameManager.h"
#include "GameObject.h"

namespace Game{

    class Snake{
        
        public:

            //model related
            Snake(std::shared_ptr<cg3d::Material> program, std::shared_ptr<cg3d::AutoMorphingModel> model, int links);
            void ResetSnake();
            int GetNumOfLinks();
            std::shared_ptr<cg3d::AutoMorphingModel> GetModel();
            static std::shared_ptr<Snake> CreateSnake(std::shared_ptr<cg3d::Material> program, std::shared_ptr<cg3d::AutoMorphingModel> model, int links);
            bool shouldAnimate;
            std::vector<std::shared_ptr<GameObject>> links;
            //game related
            void GetHit();
            void Die();
                //TODO support pickups
            void AddVelocity(float amount, cg3d::Model::Axis axis);
            void StopMoving();
            float GetMoveSpeed();
            bool IsColliding();
            Eigen::Vector3f GetMoveDirection();
            std::shared_ptr<cg3d::AutoMorphingModel> autoSnake;

        private:
            
            std::shared_ptr<cg3d::Material> snakeProgram;
            void InitSnake();
            int numOfLinks;

            //game attributes
            std::shared_ptr<Game::GameManager> gameManager;

            float maxHealth;
            float currHealth;
            float moveSpeed;
            float velocityY;
            float velocityX;
            float velocityZ;
            bool isAlive;
            bool isColliding;

        

            void InitGameValues();
            void DecreaseHealth(float amount);
            void IncreaseHealth(float amount);


    };

}