#pragma once

#include <Eigen/Geometry>
#include <utility>
#include "Movable.h"
#include "AutoMorphingModel.h"
#include <vector>
#include <string>
#include <cstdint>
#include "GameManager.h"




class Snake{
    
    public:

        //model related
        Snake(std::shared_ptr<cg3d::Material> program, std::shared_ptr<cg3d::Mesh> mesh, int links);
        void ResetSnake();
        int GetNumOfLinks();
        std::shared_ptr<cg3d::AutoMorphingModel> GetModel();
        static std::shared_ptr<Snake> CreateSnake(std::shared_ptr<cg3d::Material> program, std::shared_ptr<cg3d::Mesh> mesh, int links);
        bool shouldAnimate;
        
        //game related
        void GetHit();
        void Die();
            //TODO support pickups
                

    private:
        
        std::shared_ptr<cg3d::Material> snakeProgram;
        std::shared_ptr<cg3d::Mesh> snakeMesh;
        std::shared_ptr<cg3d::AutoMorphingModel> autoSnake;
        void InitSnake();
        int numOfLinks;

        //game attributes
        GameManager gameManager;

        float maxHealth;
        float currHealth;
        float moveSpeed;
        bool isAlive;

        void InitGameValues();
        void DecreaseHealth(float amount);
        void IncreaseHealth(float amount);




};
