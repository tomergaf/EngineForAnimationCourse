#pragma once

#include <utility>
#include "AutoMorphingModel.h"
#include <vector>
#include <string>
#include <cstdint>

#include "igl/per_vertex_normals.h"
#include <igl/AABB.h>
#include "SnakeGame.h"

namespace Game{
    
    class GameObject {
        public:
            GameObject(std::shared_ptr<cg3d::Material> material, std::shared_ptr<cg3d::Model> model, SnakeGame* scene);
            GameObject();
            void InitCollider();
            bool CollidingWith(std::shared_ptr<Game::GameObject> otherObject);
            bool ObjectsCollided(igl::AABB<Eigen::MatrixXd, 3> *boxTreeA, igl::AABB<Eigen::MatrixXd, 3> *boxTreeB, std::shared_ptr<Game::GameObject> thisObject, std::shared_ptr<Game::GameObject> otherObject);
            void Collide();
            void Trigger();
            void Hide();
            std::shared_ptr<cg3d::Material> material;
            std::shared_ptr<cg3d::Model> model;
            std::shared_ptr<GameObject> thisShared;
            igl::AABB<Eigen::MatrixXd, 3> colliderBoxTree;
            std::string name;
            SnakeGame* scene;
            
            bool isActive;
            bool permaGone;
            int timeout;
            int ticks;
            int cycles;

        protected:
            void DrawBox(Eigen::AlignedBox<double, 3> &box, int color, std::shared_ptr<cg3d::Model> model);
            bool BoxesIntersect(Eigen::AlignedBox<double, 3> &boxA, Eigen::AlignedBox<double, 3> &boxB, std::shared_ptr<Game::GameObject> thisObject, std::shared_ptr<Game::GameObject> otherObject);
            bool AdvanceTime();

            virtual void Reactivate();
    };
}