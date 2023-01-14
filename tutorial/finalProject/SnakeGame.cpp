#include "SnakeGame.h"

#include <utility>
#include <vector>

#include "ObjLoader.h"
#include "AutoMorphingModel.h"
#include "SceneWithImGui.h"
#include "CamModel.h"
#include "Visitor.h"
#include "IglMeshLoader.h"
#include "Utility.h"
#include "Snake.h"

#include "imgui.h"
#include "file_dialog_open.h"
#include "GLFW/glfw3.h"


using namespace cg3d;

void SnakeGame::BuildImGui()
{
    int flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    bool* pOpen = nullptr;

    ImGui::Begin("Menu", pOpen, flags);
    ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetWindowSize(ImVec2(0, 0), ImGuiCond_Always);
    if (ImGui::Button("Load object"))
        LoadObjectFromFileDialog();

    ImGui::Text("Camera: ");
    for (int i = 0; i < camList.size(); i++) {
        ImGui::SameLine(0);
        bool selectedCamera = camList[i] == camera;
        if (selectedCamera)
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        if (ImGui::Button(std::to_string(i + 1).c_str()))
            SetCamera(i);
        if (selectedCamera)
            ImGui::PopStyleColor();
    }
    ImGui::SameLine();
    if (ImGui::Button("Center"))
        camera->SetTout(Eigen::Affine3f::Identity());
    if (pickedModel) {
        ImGui::Text("Picked model: %s", pickedModel->name.c_str());
        ImGui::SameLine();
        if (ImGui::Button("Drop"))
            pickedModel = nullptr;
        if (pickedModel) {
            if (ImGui::CollapsingHeader("Draw options", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("Show wireframe", &pickedModel->showWireframe);
                if (pickedModel->showWireframe) {
                    ImGui::Text("Wireframe color:");
                    ImGui::SameLine();
                    ImGui::ColorEdit4("Wireframe color", pickedModel->wireframeColor.data(), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                }
                ImGui::Checkbox("Show faces", &pickedModel->showFaces);
                ImGui::Checkbox("Show textures", &pickedModel->showTextures);
                if (ImGui::Button("Scale down"))
                    pickedModel->Scale(0.9f);
                ImGui::SameLine();
                if (ImGui::Button("Scale up"))
                    pickedModel->Scale(1.1f);
            }
            if (ImGui::Button("Dump model mesh data")) {
                std::cout << "model name: " << pickedModel->name << std::endl;
                if (pickedModel->meshIndex > 0)
                    std::cout << "mesh index in use: " << pickedModel->meshIndex;
                for (auto& mesh: pickedModel->GetMeshList()) {
                    Eigen::IOFormat simple(Eigen::StreamPrecision, Eigen::DontAlignCols, ", ", ", ", "", "", "", "");
                    std::cout << "mesh name: " << mesh->name << std::endl;
                    for (int i = 0; i < mesh->data.size(); i++) {
                        if (mesh->data.size() > 1)
                            std::cout << "mesh #" << i + 1 << ":" << std::endl;
                        DumpMeshData(simple, mesh->data[i]);
                    }
                }
            }
            if (ImGui::Button("Dump model transformations")) {
                Eigen::IOFormat format(2, 0, ", ", "\n", "[", "]");
                const Eigen::Matrix4f& transform = pickedModel->GetAggregatedTransform();
                std::cout << "Tin:" << std::endl << pickedModel->Tin.matrix().format(format) << std::endl
                          << "Tout:" << std::endl << pickedModel->Tout.matrix().format(format) << std::endl
                          << "Transform:" << std::endl << transform.matrix().format(format) << std::endl
                          << "--- Transform Breakdown ---" << std::endl
                          << "Rotation:" << std::endl << Movable::GetTranslation(transform).matrix().format(format) << std::endl
                          << "Translation:" << std::endl << Movable::GetRotation(transform).matrix().format(format) << std::endl
                          << "Rotation x Translation:" << std::endl << Movable::GetTranslationRotation(transform).matrix().format(format)
                          << std::endl << "Scaling:" << std::endl << Movable::GetScaling(transform).matrix().format(format) << std::endl;
            }
        }
    }

    ImGui::End();
}

void SnakeGame::DumpMeshData(const Eigen::IOFormat& simple, const MeshData& data)
{
    std::cout << "vertices mesh: " << data.vertices.format(simple) << std::endl;
    std::cout << "faces mesh: " << data.faces.format(simple) << std::endl;
    std::cout << "vertex normals mesh: " << data.vertexNormals.format(simple) << std::endl;
    std::cout << "texture coordinates mesh: " << data.textureCoords.format(simple) << std::endl;
}

SnakeGame::SnakeGame(std::string name, Display* display) : SceneWithImGui(std::move(name), display)
{
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 5.0f;
}

void SnakeGame::SetCamera(int index)
{
    camera = camList[index];
    viewport->camera = camera;
}

void SnakeGame::Init(float fov, int width, int height, float near, float far)
{
    // create the basic elements of the scene
    AddChild(root = Movable::Create("root")); // a common (invisible) parent object for all the shapes
    auto program = std::make_shared<Program>("shaders/basicShader"); 
    // auto program = std::make_shared<Program>("shaders/phongShader"); 
    carbon = std::make_shared<Material>("carbon", program); // default material
    carbon->AddTexture(0, "textures/carbon.jpg", 2);

    //init scene 

    auto bricks{std::make_shared<Material>("bricks", program)};
    auto grass{std::make_shared<Material>("grass", program)};
    auto daylight{std::make_shared<Material>("daylight", "shaders/cubemapShader")};

    bricks->AddTexture(0, "textures/bricks.jpg", 2);
    grass->AddTexture(0, "textures/grass.bmp", 2);
    daylight->AddTexture(0, "textures/cubemaps/Daylight Box_", 3);

    auto background{Model::Create("background", Mesh::Cube(), daylight)};
    AddChild(background);

    //init snake object
    auto snakeMesh{ObjLoader::MeshFromObjFiles<std::string>("snakeMesh", "data/snake2.obj")};
    auto snake = Snake::CreateSnake(grass, snakeMesh, 16);
     
    root->AddChild(snake->GetModel());

    // create the camera objects

    //First Person Camera
    camList.resize(camList.capacity());
    camList[0] = Camera::Create("camera0", fov, float(width) / float(height), near, far);
    camList[0]->Translate(-1, Axis::X);
    camList[0]->RotateByDegree(90, Axis::Y);
    snake->GetModel()->AddChild(camList[0]);
    
    // top down camera
    for (int i = 1; i < camList.size(); i++) {
        auto camera = Camera::Create("", fov, double(width) / height, near, far);
        auto camMesh{IglLoader::MeshFromFiles("camMesh", "data/camera.obj")};
        auto model = Model::Create(std::string("camera") + std::to_string(i), camMesh, bricks);
        // auto model = ObjLoader::ModelFromObj(std::string("camera") + std::to_string(i), "data/camera.obj", carbon);
        ShowObject(model, false);
        root->AddChild(camList[i] = CamModel::Create(*camera, *model));
    }


    camList[1]->Translate(30, Axis::Y);
    camList[1]->RotateByDegree(-90, Axis::X);
    camera = camList[1];
    

    cube1 = Model::Create("cube1", Mesh::Cube(), bricks);
    cube2 = Model::Create("cube2", Mesh::Cube(), bricks);
    cube1->Translate({-3, 0, -5});
    cube2->Translate({3, 0, -5});
    root->AddChildren({cube1, cube2});

    background->Scale(120, Axis::XYZ);
    background->SetPickable(false);
    background->SetStatic();
}

void SnakeGame::Update(const Program& p, const Eigen::Matrix4f& proj, const Eigen::Matrix4f& view, const Eigen::Matrix4f& model)
{
    Scene::Update(p, proj, view, model);
    p.SetUniform4f("lightColor", 0.8f, 0.3f, 0.0f, 0.5f);
    p.SetUniform4f("Kai", 1.0f, 0.3f, 0.6f, 1.0f);
    p.SetUniform4f("Kdi", 0.5f, 0.5f, 0.0f, 1.0f);
    p.SetUniform1f("specular_exponent", 5.0f);
    p.SetUniform4f("light_position", 0.0, 15.0f, 0.0, 1.0f);
    if (animate) {
       
    }
}

void SnakeGame::LoadObjectFromFileDialog()
{
    std::string filename = igl::file_dialog_open();
    if (filename.length() == 0) return;

    auto shape = Model::Create(filename, carbon);
}

void SnakeGame::KeyCallback(Viewport* _viewport, int x, int y, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        if (key == GLFW_KEY_SPACE)
            SetActive(!IsActive());

        // keys 1-9 are objects 1-9 (objects[0] - objects[8]), key 0 is object 10 (objects[9])
        if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
            if (int index; (index = (key - GLFW_KEY_1 + 10) % 10) < camList.size())
                SetCamera(index);
        }
    }

    SceneWithImGui::KeyCallback(nullptr, x, y, key, scancode, action, mods);
}

void SnakeGame::ViewportSizeCallback(Viewport* _viewport)
{
    for (auto& cam: camList)
        cam->SetProjection(float(_viewport->width) / float(_viewport->height));

    // note: we don't need to call Scene::ViewportSizeCallback since we are setting the projection of all the cameras
}

void SnakeGame::AddViewportCallback(Viewport* _viewport)
{
    viewport = _viewport;

    Scene::AddViewportCallback(viewport);
}

void SnakeGame::ShowObject(std::shared_ptr<cg3d::Model> model, bool dir){
    model->showFaces = dir;
    model->showWireframe = dir;
    model->showTextures = dir;
    model->isHidden = !dir;
}