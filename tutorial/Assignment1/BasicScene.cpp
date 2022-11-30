#include "BasicScene.h"
#include <read_triangle_mesh.h>
#include <utility>
#include "ObjLoader.h"
#include "IglMeshLoader.h"
#include "igl/read_triangle_mesh.cpp"
#include "igl/per_vertex_normals.h"
#include "igl/edge_flaps.h"
#include <igl/circulation.h>
#include <igl/collapse_edge.h>
#include <igl/edge_flaps.h>
#include <igl/decimate.h>
#include <igl/shortest_edge_and_midpoint.h>
#include <igl/parallel_for.h>
#include <igl/read_triangle_mesh.h>
#include <igl/opengl/glfw/Viewer.h>
#include <Eigen/Core>
#include <iostream>
#include <vector>
#include "AutoMorphingModel.h"

#include "Visitor.h"
#include "Utility.h"

#include "imgui.h"
#include "file_dialog_open.h"
#include "GLFW/glfw3.h"

#include <set>

using namespace cg3d;
using namespace std;
using namespace Eigen;
using namespace igl;

void BasicScene::Init(float fov, int width, int height, float near, float far)
{
	camera = Camera::Create("camera", fov, float(width) / float(height), near, far);
	auto program = std::make_shared<Program>("shaders/basicShader");
	auto material = std::make_shared<Material>("material", program); // empty material
	auto daylight{std::make_shared<Material>("daylight", "shaders/cubemapShader")};
	daylight->AddTexture(0, "textures/cubemaps/Daylight Box_", 3);
	auto background{Model::Create("background", Mesh::Cube(), daylight)};

	AddChild(root = Movable::Create("root")); // a common (invisible) parent object for all the shapes

	AddChild(background);
	background->Scale(120, Axis::XYZ);
	background->SetPickable(false);
	background->SetStatic();

	material->AddTexture(0, "textures/box0.bmp", 2);
	auto sphereMesh{IglLoader::MeshFromFiles("sphere_igl", "data/sphere.obj")};
	auto complexMesh{IglLoader::MeshFromFiles("complex", "data/fertility.off")};
	auto cubeMesh{IglLoader::MeshFromFiles("cube_igl", "data/cube.off")};

	// morphing function - cycles through mesh indices by comparing to times pressed space
	auto morphFunc = [=](Model *model, cg3d::Visitor *visitor)
	{
		int cycle = clickMap[model] % model->GetMesh()->data.size(); // curr cycle is number of times space pressed
		// modulo is so to get a valid number of mesh to choose -> 13 clicks -> mesh 3
		return cycle;
	};
	
	decimationMult = 0.05; //the modifier to decimate with - change this to increase / decrease edge decimation

	// pre decimate mesh so user can cycle at runtime
	PreDecimateMesh(sphereMesh);
	PreDecimateMesh(complexMesh); 
	PreDecimateMesh(cubeMesh);

	// model creation for automotphing
	sphere1 = Model::Create("sphere", sphereMesh, material);
	complex = Model::Create("cyl", complexMesh, material);
	cube = Model::Create("cube", cubeMesh, material);

	// auto morphing model creation
	auto autoSphere1 = AutoMorphingModel::Create(*sphere1, morphFunc);
	auto autoCube = AutoMorphingModel::Create(*cube, morphFunc);
	auto autoComplex = AutoMorphingModel::Create(*complex, morphFunc);

	// object setup
	camera->Translate(20, Axis::Z);
	autoSphere1->showWireframe = true;
	autoComplex->showWireframe = true;
	autoCube->showWireframe = true;
	autoSphere1->Scale(2);
	autoComplex->Scale(0.035f);
	autoSphere1->Translate({-3, 0, 0});
	autoComplex->Translate({4, 0, 0});
	clickMap.insert({sphere1.get(), 0});
	clickMap.insert({complex.get(), 0});
	clickMap.insert({cube.get(), 0});
	root->AddChild(autoSphere1);
	root->AddChild(autoComplex);
	root->AddChild(autoCube);
}

void BasicScene::PreDecimateMesh(std::shared_ptr<cg3d::Mesh> mesh)
{
	MatrixXd V, OV; //matrices for data
	MatrixXi F, OF;
	double initialSize; //size for initial edge queue

	OV = mesh->data.back().vertices; //initial data from loaded mesh
	OF = mesh->data.back().faces;

	Eigen::VectorXi EMAP;
	Eigen::MatrixXi E, EF, EI;
	Eigen::VectorXi EQ;
	igl::min_heap<std::tuple<double, int, int>> Q; //priority queue for costs

	MatrixXd C; //cost matrices
	int num_collapsed; //counter of collapsed edges

#pragma region reset
	auto reset = [&]()
	{
		F = OF;
		V = OV;
		edge_flaps(F, E, EMAP, EF, EI);
		C.resize(E.rows(), V.cols());
		VectorXd costs(E.rows());
		// https://stackoverflow.com/questions/2852140/priority-queue-clear-method
		// Q.clear();

		Q = {};
		EQ = Eigen::VectorXi::Zero(E.rows());
		{
			Eigen::VectorXd costs(E.rows());
			igl::parallel_for(
				E.rows(), [&](const int e)
				{
						double cost = e;
						RowVectorXd p(1, 3);
						shortest_edge_and_midpoint(e, V, F, E, EMAP, EF, EI, cost, p);
						C.row(e) = p;
						costs(e) = cost; },
				10000);
			for (int e = 0; e < E.rows(); e++)
			{
				Q.emplace(costs(e), e, 0);
			}
		}
		initialSize = Q.size(); //initial size is the first edge queue
		debug_print(mesh->name);
		debug_print(initialSize);
		num_collapsed = 0; 
	};

#pragma endregion

#pragma region collapse
	auto cutedges = [&]()
	{
		bool collapsed = false;
		if (!Q.empty())
		{
			// collapse edge
			// the magic number changes how much edges collapse
			const int max_iter = std::ceil(decimationMult * initialSize);
			for (int j = 0; j < max_iter; j++)
			{
				if (!collapse_edge(shortest_edge_and_midpoint, V, F, E, EMAP, EF, EI, Q, EQ, C))
				{
					break;
				}
				collapsed = true;
				num_collapsed++;
				initialSize--; //edges removed are decreased from total to remove
			}
			// get the missing vars for a mesh
			if(collapsed){
				Eigen::MatrixXd VN;
				igl::per_vertex_normals(V, F, VN);
				Eigen::MatrixXd TC = Eigen::MatrixXd::Zero(V.rows(), 2);
				mesh->data.push_back(MeshData{V, F, VN, TC});
				debug_print(num_collapsed);
			}
		}
	};

#pragma endregion

	//the actual flow - first reset vals, then cut edges 10 times
	reset();
	for (int i = 0; i < 10; i++)
	{
		cutedges();
	}
}

/*
STOPPED HERE:
- need to check how many edges are being removed - TODO - edges are counted, add which model are they from
- still need to make another collapse edge function as requested - TODO - use the algorithm from the lecture to replace the collapse edges algorithm supplied
*/

void BasicScene::updateModelClickCount(bool direction)
{
	if (pickedModel)
	{
		if (clickMap.find(pickedModel.get()) != clickMap.end())
		{
			if (direction)
				clickMap[pickedModel.get()] += 1;
			else
			{
				if (clickMap[pickedModel.get()] >= 1)
					clickMap[pickedModel.get()] -= 1;
			}
		}
	}
}

void BasicScene::Update(const Program &program, const Eigen::Matrix4f &proj, const Eigen::Matrix4f &view, const Eigen::Matrix4f &model)
{
	Scene::Update(program, proj, view, model);
}

void BasicScene::KeyCallback(Viewport *_viewport, int x, int y, int key, int scancode, int action, int mods)
{
	// TODO make this aware of picked object
	if (action == GLFW_PRESS || action == GLFW_REPEAT)
	{
		if (key == GLFW_KEY_SPACE)
			updateModelClickCount(true);
		if (key == GLFW_KEY_UP)
			updateModelClickCount(true);
		if (key == GLFW_KEY_DOWN)
			updateModelClickCount(false);
		if (key == GLFW_KEY_R)
		{
			// reset all click counts
			for (auto &key : clickMap)
			{
				key.second = 0;
			}
		}
	}
}