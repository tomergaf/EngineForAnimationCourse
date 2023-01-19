#pragma once

#include <Eigen/Geometry>
#include <utility>
#include "Movable.h"
#include "AutoMorphingModel.h"
#include <vector>
#include <string>
#include <cstdint>
#include "GameManager.h"

namespace Game{
    
    class GameObject {
        public:

            GameObject(std::shared_ptr<cg3d::Material> material, std::shared_ptr<cg3d::Model> model);
            GameObject();
            std::shared_ptr<cg3d::Material> material;
            std::shared_ptr<cg3d::Model> model;


            
            
            bool isActive;

            void Collide();
            void Trigger();
            void Hide();


        private:


    };
}