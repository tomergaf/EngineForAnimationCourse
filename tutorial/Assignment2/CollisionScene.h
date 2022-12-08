#pragma once

#include "Scene.h"

#include <utility>
#include <igl/AABB.h>
#include <vector>
#include <Eigen/Core>

class CollisionScene : public cg3d::Scene
{

    
public:
    explicit CollisionScene(std::string name, cg3d::Display* display) : Scene(std::move(name), display) {};
    void Init(float fov, int width, int height, float near, float far);
    void Update(const cg3d::Program& program, const Eigen::Matrix4f& proj, const Eigen::Matrix4f& view, const Eigen::Matrix4f& model) override;
    void KeyCallback(cg3d::Viewport* _viewport, int x, int y, int key, int scancode, int action, int mods) override;
    void UpdateModelClickCount(bool direction);
    void PreDecimateMesh(std::shared_ptr<cg3d::Mesh> mesh,bool custom);
    void AnimateUntilCollision(std::shared_ptr<cg3d::Model> model, float velocity);
    void DrawBox(Eigen::AlignedBox<double, 3>& box, int color, std::shared_ptr<cg3d::Model> model);
	bool ObjectsCollided(igl::AABB<Eigen::MatrixXd, 3>* boxTreeA, igl::AABB<Eigen::MatrixXd, 3>* boxTreeB, std::shared_ptr<cg3d::Model> model, std::shared_ptr<cg3d::Model> otherModel);
	bool BoxesIntersect(Eigen::AlignedBox<double, 3>& boxA, Eigen::AlignedBox<double, 3>& boxB , std::shared_ptr<cg3d::Model> model, std::shared_ptr<cg3d::Model> otherModel);

private:
    std::shared_ptr<cg3d::Movable> root;
    double decimationMult;
    float velocity;
    bool objectsCollided;
    std::map<cg3d::Model*, int> clickMap;
    igl::AABB<Eigen::MatrixXd, 3> objectTree1, objectTree2;
    std::shared_ptr<cg3d::Model> staticBunny, movingBunny;
    std::shared_ptr<cg3d::Model> collisionCube1, collisionCube2;
    std::vector<igl::AABB<Eigen::MatrixXd, 3>> boxTrees;
    std::vector<Eigen::MatrixXd> Ve;
	std::vector<Eigen::MatrixXi> Fa;
};
