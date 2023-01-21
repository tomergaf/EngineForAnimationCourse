#pragma once

#include "GameObject.h"
#include <random>
#include "igl/per_vertex_normals.h"
#include "SnakeGame.h"


namespace Game{

class MovingObject: public GameObject{

    public:
        explicit MovingObject(std::shared_ptr<cg3d::Material> material, std::shared_ptr<cg3d::Model> model, SnakeGame* scene);
        float GetSpeed();
        void  GenerateBezierCurve();
        void  DrawCurve();
        void  Move();
        void SetTimeOut();
        Eigen::Vector3f  MoveBezier();

        Eigen::Matrix4f M;
        Eigen::Matrix <float, 4, 3 > curvePoints;
        Eigen::Matrix <float, 4, 3 > MG_Result;

        Eigen::RowVector4f T;
        std::vector<Eigen::Vector3d> spawnLocations;
        Eigen::Vector3f currentPosition;

        bool moveBackwards;
        float t;
        float xCoordinate;
        float yCoordinate;
        float zCoordinate;

        virtual void  Update() = 0;
        virtual void  OnCollision() = 0;
        float speed;
    private:

};

}