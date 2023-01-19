#pragma once

#include "SceneWithImGui.h"
#include "CamModel.h"

#include <memory>
#include <utility>
#include "Snake.h"




    class SnakeGame : public cg3d::SceneWithImGui
    {
    private:
        std::shared_ptr<Game::Snake> snake;
        std::shared_ptr<Movable> root;
        std::shared_ptr<cg3d::Material> carbon;
        bool animate;
        void BuildImGui() override;
        std::vector<std::shared_ptr<cg3d::Camera>> camList{2};
        std::shared_ptr<cg3d::Model> cube1, cube2, cylinder, sphere1, sphere2;
        cg3d::Viewport* viewport = nullptr;

    public:
        float velInterval;
        SnakeGame(std::string name, cg3d::Display* display);
        void Init(float fov, int width, int height, float near, float far);
        void AnimateUntilCollision(std::shared_ptr<Game::Snake> snakeModel);
        void Update(const cg3d::Program &program, const Eigen::Matrix4f &proj, const Eigen::Matrix4f &view, const Eigen::Matrix4f &model) override;
        void KeyCallback(cg3d::Viewport* _viewport, int x, int y, int key, int scancode, int action, int mods) override;
        void AddViewportCallback(cg3d::Viewport* _viewport) override;
        void ViewportSizeCallback(cg3d::Viewport* _viewport) override;

    private:
        void ShowObject(std::shared_ptr<cg3d::Model> model, bool dir);
        inline bool IsActive() const { return animate; };
        inline void SetActive(bool _isActive = true) { animate = _isActive; }
        void LoadObjectFromFileDialog();
        void UpdateXVelocity(bool direction);
        void UpdateYVelocity(bool direction);
        void UpdateZVelocity(bool direction);
        void StopMotion();
        void SetCamera(int index);
        static std::shared_ptr<CamModel> CreateCameraWithModel(int width, int height, float fov, float near, float far, const std::shared_ptr<cg3d::Material>& material);
        static void DumpMeshData(const Eigen::IOFormat& simple, const cg3d::MeshData& data) ;
    };


