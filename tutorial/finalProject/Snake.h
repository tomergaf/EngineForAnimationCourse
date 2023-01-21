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
#include "SnakeGame.h"


namespace Game{

    class Snake{
        
        public:

            //model related
            Snake(std::shared_ptr<cg3d::Material> program, std::shared_ptr<cg3d::AutoMorphingModel> model, int links, SnakeGame* scene);
            void ResetSnake();
            int GetNumOfLinks();
            void GetHit(float amount);
            void Die();
            void AddVelocity(float amount, cg3d::Model::Axis axis);
            void StopMoving();
            float GetMoveSpeed();
            bool IsColliding();
            void InitSnake();
            void IncreaseHealth(float amount);
            void DecreaseHealth(float amount);
            std::shared_ptr<cg3d::AutoMorphingModel> GetModel();
            static std::shared_ptr<Snake> CreateSnake(std::shared_ptr<cg3d::Material> program, std::shared_ptr<cg3d::AutoMorphingModel> model, int links, SnakeGame* scene);
            bool shouldAnimate;
            std::vector<std::shared_ptr<GameObject>> links;
            //game related
            Eigen::Vector3f GetMoveDirection();
            std::shared_ptr<cg3d::AutoMorphingModel> autoSnake;
            SnakeGame* scene;

        private:
            
            std::shared_ptr<cg3d::Material> snakeProgram;
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



    };

}