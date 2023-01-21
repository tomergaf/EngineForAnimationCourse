#include "SnakeGame.h"

#include <utility>
#include <vector>

#include "igl/vertex_triangle_adjacency.h"
#include "igl/per_face_normals.h"
#include "igl/edge_flaps.h"
#include <igl/circulation.h>
#include <igl/collapse_edge.h>
#include <igl/edge_flaps.h>
#include <igl/decimate.h>
#include <igl/shortest_edge_and_midpoint.h>
#include <igl/parallel_for.h>
#include <igl/read_triangle_mesh.h>
#include <igl/opengl/glfw/Viewer.h>

#include "ObjLoader.h"
#include "AutoMorphingModel.h"
#include "SceneWithImGui.h"
#include "CamModel.h"
#include "Visitor.h"
#include "IglMeshLoader.h"
#include "Utility.h"
#include "Snake.h"
#include "MovingObject.h"
#include "Pickup.h"
#include "HealthPickup.h"
#include "Obstacle.h"
#include "Snake.h"
#include "GameManager.h"
#include "GameObject.h"

#include "imgui.h"
#include "file_dialog_open.h"
#include "GLFW/glfw3.h"

//TEMP
#define DECIMATION_MULT 0.5

using namespace std;
using namespace Eigen;
using namespace igl;
using namespace cg3d;
using namespace Game;

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

void SnakeGame::PreDecimateMesh(std::shared_ptr<cg3d::Mesh> mesh, bool custom)
{
    float decimationMult = DECIMATION_MULT;
	Eigen::MatrixXd V, OV, VN, FN; // matrices for data
	Eigen::MatrixXi F, OF;
	double initialSize; // size for initial edge queue

	// for custom collapse
	std::vector<Eigen::Matrix4d> Qmatrices;
	// compute q matrices for all verticies

	OV = mesh->data.back().vertices; // initial data from loaded mesh
	OF = mesh->data.back().faces;

	Eigen::VectorXi EMAP;
	Eigen::MatrixXi E, EF, EI;
	Eigen::VectorXi EQ;
	igl::min_heap<std::tuple<double, int, int>> Q; // priority queue for costs

	Eigen::MatrixXd C;		   // cost matrices
	int num_collapsed; // counter of collapsed edges

#pragma region customCollapse
	auto computeQ = [&]() 
	{
		// this is used to initialize the Q matrices of each vertex 
		std::vector<std::vector<int>> V2F; // vertex to face vector
		std::vector<std::vector<int>> NI;  // for adj

		int n = V.rows();
		Qmatrices.resize(n);
		igl::vertex_triangle_adjacency(n, F, V2F, NI);
		igl::per_face_normals(V, F, FN);
		for (int i = 0; i < n; i++)
		{ // for each vertex in q
			Qmatrices[i] = Eigen::Matrix4d::Zero();
			for (int j = 0; j < V2F[i].size(); j++)
			{														   // for each vertex face
				Eigen::Vector3d norm = FN.row(V2F[i][j]).normalized(); // get the face normal
				Eigen::Vector4d p;
				p << norm, (V.row(i) * norm) * -1;
				Eigen::Matrix4d Kp = p * p.transpose(); // Kp = pp^T
				Qmatrices[i] += Kp; //from the given formula
			}
		}
	};

	// select all valid pairs and compute optimal contraction for target v' for each (v1,v2)
	auto costCalc = [&](
						const int edge,
						const Eigen::MatrixXd &V,
						const Eigen::MatrixXi & /*F*/,
						const Eigen::MatrixXi &E,
						const Eigen::VectorXi & /*EMAP*/,
						const Eigen::MatrixXi & /*EF*/,
						const Eigen::MatrixXi & /*EI*/,
						double &cost,
						Eigen::RowVectorXd &p)
	{
		int v1 = E(edge, 0); // get vertices of edge
		int v2 = E(edge, 1);
		
		std::pair<Eigen::Vector4d, int> mincost;
		Eigen::Matrix4d Qtag = Qmatrices[v1] + Qmatrices[v2];
		Eigen::Matrix4d Qcoor = Qtag;
		Eigen::Vector4d vtag;

		Qcoor.row(3) = Eigen::Vector4d(0, 0, 0, 1);
		bool inversable;
		Qcoor.computeInverseWithCheck(Qcoor, inversable);
		if (inversable)
		{
			vtag = Qcoor * (Eigen::Vector4d(0, 0, 0, 1));
			cost = vtag.transpose() * Qtag * vtag;
		}
		else
		{
			Eigen::Vector4d v1coor;
			Eigen::Vector4d v2coor;
			Eigen::Vector4d v1v2coor;
			v1coor << V.row(v1).x(),V.row(v1).y(), V.row(v1).z() , 1; // make into 4
			v2coor << V.row(v2).x(),V.row(v2).y(), V.row(v2).z() , 1; // make into 4
			Eigen::Vector3d v1v2 = (V.row(v1) + V.row(v2)) / 2 ;
			v1v2coor << v1v2 , 1; // make into 4
			double cost1 = v1coor.transpose() * Qtag * v1coor;
			double cost2 = v2coor.transpose() * Qtag * v2coor;
			double cost3 = v1v2coor.transpose() * Qtag * v1v2coor;
			mincost = std::min({pair(v1coor, cost1), pair(v2coor, cost2), pair(v1v2coor, cost3)}, [](pair<Eigen::Vector4d, int> a, pair<Eigen::Vector4d, int> b)
							   { return a.second > b.second; }); //calculate minimal error
			
			cost = mincost.second;
			vtag = mincost.first;
		}
		Eigen::Vector3d vtagCoor;
		vtagCoor << vtag[0], vtag[1], vtag[2];
		p = vtagCoor;
	};

#pragma endregion

#pragma region reset
	auto reset = [&](bool custom)
	{
		F = OF;
		V = OV;
		edge_flaps(F, E, EMAP, EF, EI);
		C.resize(E.rows(), V.cols());
		VectorXd costs(E.rows());
		if (custom)
			computeQ();
		Q = {};
		EQ = Eigen::VectorXi::Zero(E.rows());
		{
			Eigen::VectorXd costs(E.rows());
			igl::parallel_for(
				E.rows(), [&](const int e)
				{
						double cost = e;
						RowVectorXd p(1, 3);
						// decimation algorithm chosen by flag
						if(!custom)
							shortest_edge_and_midpoint(e, V, F, E, EMAP, EF, EI, cost, p);
						else {
							costCalc(e, V, F, E, EMAP, EF, EI, cost, p);
						}
						C.row(e) = p;
						costs(e) = cost; },
				10000);
			for (int e = 0; e < E.rows(); e++)
			{
				Q.emplace(costs(e), e, 0);
			}
		}
		initialSize = Q.size(); // initial size is the first edge queue
		debug_print(mesh->name, " initial edge count: ", initialSize);
		num_collapsed = 0;
	};

#pragma endregion

#pragma region collapse
	auto cutedges = [&](bool custom)
	{
		bool collapsed = false;
		if (!Q.empty())
		{
			// collapse edge
			if(custom)
				computeQ(); //compute q matrix after each round of collapsing - better result without this for simple shapes
			const int max_iter = std::ceil(decimationMult * initialSize);
			for (int j = 0; j < max_iter; j++)
			{
				Eigen::RowVectorXd pos(1, 3);
				double cost = std::get<0>(Q.top());
				int e = std::get<1>(Q.top());
				if (custom)
				{
					// UNCOMMENT TO GET PRINTS
					// costCalc(e, V, F, E, EMAP, EF, EI, cost, pos);
					if (!collapse_edge(costCalc, V, F, E, EMAP, EF, EI, Q, EQ, C)) // use calculation according to flag
						break;
				}
				else
				{
					// UNCOMMENT TO GET PRINTS
					// costCalc(e, V, F, E, EMAP, EF, EI, cost, pos);
					if (!collapse_edge(shortest_edge_and_midpoint, V, F, E, EMAP, EF, EI, Q, EQ, C))
						break;
				}
				// UNCOMMENT TO GET PRINTS

				// cout << "Edge removed is: " << e
				// 	 << ", Cost was: " << cost
				// 	 << ", New v position is: (" << pos[0] << "," << pos[1] << "," << pos[2] << ")"
				// 	 << std::endl;

				collapsed = true;
				num_collapsed++;
				initialSize--; // edges removed are decreased from total to remove
			}
			// get the missing vars for a mesh
			if (collapsed)
			{
				Eigen::MatrixXd VN;
				igl::per_vertex_normals(V, F, VN);
				Eigen::MatrixXd TC = Eigen::MatrixXd::Zero(V.rows(), 2);
				mesh->data.push_back(MeshData{V, F, VN, TC});
				debug_print("collapses successful so far: ", num_collapsed);
			}
		}
	};

#pragma endregion

	// the actual flow - first reset vals, then cut edges 10 times
	reset(custom);
	for (int i = 0; i < 10; i++)
	{
		cutedges(custom);
	}
}

void SnakeGame::Init(float fov, int width, int height, float near, float far)
{
    //init manager
    GameManager* gameManagerPtr = new GameManager();
    gameManager = std::make_shared<GameManager>(*gameManagerPtr);
    //TEMP
    velInterval =0.1f;
    animate = true;
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
    auto snakeMesh{ObjLoader::MeshFromObjFiles<std::string>("snakeMesh", "data/snake1.obj")};
    snakeMesh->data.push_back(IglLoader::MeshFromFiles("cyl_igl","data/xcylinder.obj")->data[0]);
    auto snakeModel{Model::Create("snake", snakeMesh, grass)};

    auto morphFunc = [](Model* model, cg3d::Visitor* visitor) {
        return 0;
    };

    std::shared_ptr<AutoMorphingModel> autoSnake = AutoMorphingModel::Create(*snakeModel, morphFunc);
    autoSnake->showWireframe = false;
    snake = Game::Snake::CreateSnake(grass, autoSnake, 16, this);
    gameManager->snake = snake;
    root->AddChild(snake->autoSnake);

    // create the camera objects

    //First Person Camera
    camList.resize(camList.capacity());
    camList[0] = Camera::Create("camera0", fov, float(width) / float(height), near, far);
    camList[0]->Translate(-1, Axis::X);
    camList[0]->RotateByDegree(90, Axis::Y);
    snake->autoSnake->AddChild(camList[0]);
    
    // top down camera
    for (int i = 1; i < camList.size(); i++) {
        auto camera = Camera::Create("", fov, double(width) / height, near, far);
        auto camMesh{IglLoader::MeshFromFiles("camMesh", "data/camera.obj")};
        auto model = Model::Create(std::string("camera") + std::to_string(i), camMesh, bricks);
        // auto model = ObjLoader::ModelFromObj(std::string("camera") + std::to_string(i), "data/camera.obj", carbon);
        ShowObject(model, false);
        // root->AddChild(camList[i] = CamModel::Create(*camera, *model));
        snake->autoSnake->AddChild(camList[i] = CamModel::Create(*camera, *model));
    }


    camList[1]->Translate(50, Axis::Y);
    camList[1]->RotateByDegree(-90, Axis::X);
    camera = camList[1];
    

    cube1 = Model::Create("cube1", Mesh::Cube(), bricks);
    cube2 = Model::Create("cube2", Mesh::Cube(), bricks);
    cube1->Translate({-3, 0, -5});
    cube2->Translate({3, 0, -5});
    
    // TEMP item setup
    auto sphereMesh{IglLoader::MeshFromFiles("sphere1", "data/sphere.obj")};
    sphere1 = Model::Create("sphere1", sphereMesh, bricks);
    auto spherePointer = new Game::Pickup(bricks, sphere1, this);
    sphereObj = std::make_shared<Pickup>(*spherePointer);
    interactables.push_back(sphereObj);
    sphere2 = Model::Create("sphere2", sphereMesh, bricks);
    auto spherePointer2 = new Game::HealthPickup(bricks, sphere2, this);
    sphereObj2 = std::make_shared<HealthPickup>(*spherePointer2);
    interactables.push_back(sphereObj2);
    auto sphere3 = Model::Create("sphere3", sphereMesh, bricks);
    interactables.push_back(std::make_shared<Obstacle>(*(new Game::Obstacle(bricks, sphere3, this))));
   
    
    // TEMP Collisions setup
    auto collisionCube1Mesh {IglLoader::MeshFromFiles("collisioncube1mesh", "data/cube.off")};
	auto collisionCube2Mesh {IglLoader::MeshFromFiles("collisioncube2mesh", "data/cube.off")};
    collisionCube1 = Model::Create("collisioncube1", collisionCube1Mesh, bricks);
    collisionCube2 = Model::Create("collisioncube2", collisionCube2Mesh, bricks);
    collisionCube1->showFaces = false;
    collisionCube2->showFaces = false;
    collisionCube1->showWireframe = true;
    collisionCube2->showWireframe = true;
    collisionCube1->isHidden = true;
    collisionCube2->isHidden = true;


	snake->GetModel()->AddChild(collisionCube1); //add collision cube as child so it moves and rotates with parent
	sphereObj->model->AddChild(collisionCube2); 
    

    sphere1->Translate({-2, 0, 0});
    sphere2->Translate({-6, 0, 0});
    sphere3->Translate({-8, 0, 0});
    root->AddChildren({cube1, cube2, sphere1, sphere2, sphere3});

    background->Scale(120, Axis::XYZ);
    background->SetPickable(false);
    background->SetStatic();
}

void SnakeGame::AnimateUntilCollision(std::shared_ptr<Game::Snake> snakeModel){
	if(!snakeModel->IsColliding()){
        Eigen::Vector3f moveVector = snakeModel->GetMoveDirection();
		// moveVector = {-1*velocityX,1*velocityY,1*velocityZ};
		// moveVector = {-1*moveVector(0),1*moveVector(1),1*moveVector(2)}*snakeModel->GetMoveSpeed();
		moveVector = snakeModel->GetMoveDirection() * snakeModel->GetMoveSpeed();
		snakeModel->autoSnake->Translate(moveVector);
        // debug_print(moveVector);
	}
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
        
        AnimateUntilCollision(this->snake);
        //TEMP
        // sphereObj->DrawCurve();
        // Eigen::Vector3f dir = sphereObj->MoveBezier();
        ticks+=1;
        if(ticks % 2 == 0){
            for (auto & elem : interactables){

                elem->Update();
            }
        }
    }
}

void SnakeGame::LoadObjectFromFileDialog()
{
    std::string filename = igl::file_dialog_open();
    if (filename.length() == 0) return;

    auto shape = Model::Create(filename, carbon);
}

// TEMP Motion

void SnakeGame::UpdateXVelocity(bool direction){ //function for changing velocity on button click

	float change = direction ? velInterval*1 : velInterval*-1;
	snake->AddVelocity(change, Axis::X);
}

void SnakeGame::UpdateYVelocity(bool direction){
	float change = direction ? velInterval*1 : velInterval*-1;
	snake->AddVelocity(change, Axis::Y);
}

void SnakeGame::UpdateZVelocity(bool direction){
	float change = direction ? velInterval*1 : velInterval*-1;
	snake->AddVelocity(change, Axis::Z);
}

void SnakeGame::StopMotion(){
	snake->StopMoving();
}


void SnakeGame::KeyCallback(Viewport* _viewport, int x, int y, int key, int scancode, int action, int mods)
{

    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        // if (key == GLFW_KEY_SPACE)
        //     SetActive(!IsActive());

        // Temp Motion
		if (key == GLFW_KEY_SPACE)
			StopMotion();
		if (key == GLFW_KEY_UP) 
			UpdateYVelocity(true);
		if (key == GLFW_KEY_DOWN) 
			UpdateYVelocity(false);
		if (key == GLFW_KEY_LEFT) 
			UpdateXVelocity(true);
		if (key == GLFW_KEY_RIGHT) 
			UpdateXVelocity(false);
        // keys 1-9 are objects 1-9 (objects[0] - objects[8]), key 0 is object 10 (objects[9])
        if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
            if (int index; (index = (key - GLFW_KEY_1 + 10) % 10) < camList.size())
                SetCamera(index);
        }
    }

    //SceneWithImGui::KeyCallback(nullptr, x, y, key, scancode, action, mods);
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