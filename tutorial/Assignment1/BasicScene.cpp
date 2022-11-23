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

#include "SceneWithImGui.h"
#include "Visitor.h"
#include "Utility.h"

#include "imgui.h"
#include "file_dialog_open.h"
#include "GLFW/glfw3.h"

#include <set>

using namespace cg3d;
// tutorial altered
using namespace std;
using namespace Eigen;
using namespace igl;

void BasicScene::Init(float fov, int width, int height, float near, float far)
{
	currcycle = 0;
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
	auto cylMesh{IglLoader::MeshFromFiles("cyl_igl", "data/camel_b.obj")};
	auto cubeMesh{IglLoader::MeshFromFiles("cube_igl", "data/cube.off")};

	#pragma region decimate

	auto preDecimateMesh = [&](std::shared_ptr<cg3d::Mesh> mesh)
	{

		MatrixXd V, OV;
		MatrixXi F, OF;
		
		OV = mesh->data[0].vertices;
		OF = mesh->data[0].faces;

		Eigen::VectorXi EMAP;
		Eigen::MatrixXi E, EF, EI;
		Eigen::VectorXi EQ;
		igl::min_heap<std::tuple<double, int, int>> Q;

		MatrixXd C;
		int num_collapsed;

		// reset function
		#pragma region reset
		auto reset = [&]() mutable
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
				num_collapsed = 0;
			}
		};

		#pragma endregion

		// collapse function
		#pragma region collapse
		auto cutedges = [&]() mutable
		{
			if (!Q.empty())
			{
				// collapse edge
				// the magic number changes how much edges collapse
				const int max_iter = std::ceil(0.02 * Q.size());
				for (int j = 0; j < max_iter; j++)
				{
					if (!collapse_edge(shortest_edge_and_midpoint, V, F, E, EMAP, EF, EI, Q, EQ, C))
					{
						break;
					}
					num_collapsed++;
				}
				//get the missing vars for a mesh
				Eigen::MatrixXd VN;
				igl::per_vertex_normals(V, F, VN);
        		Eigen::MatrixXd TC = Eigen::MatrixXd::Zero(V.rows(),2);
				mesh->data.push_back(MeshData{V, F, VN, TC});
				debug_print(num_collapsed);
			}
		};

		#pragma endregion

		reset();
		for (int i = 0; i < 10; i++)
		{
			cutedges();
		}
	};

	#pragma endregion


	/*
	STOPPED HERE:
	- need to use a automorphingmodel to switch between meshdatas - Done
	- need to connect a callback for clicking space and increasing cycle count - Done
	- need to link picker to model mesh to cycle - TODO
	- need to tidy this up in general and have a good chronological order - TODO
	- need to check how many edges are being removed - TODO - edges are counted, add which model are they from
	- still need to make another collapse edge function as requested - TODO - use the algorithm from the lecture to replace the collapse edges algorithm supplied

	*/

	preDecimateMesh(sphereMesh);
	//preDecimateMesh(cylMesh); //TODO - figure out why this doesnt work with high magic
	preDecimateMesh(cubeMesh);

	// morphing function - cycles through mesh indices by comparing to times pressed space
	// TODO - make this aware of the clicked item instead of globally cycling all meshes
	auto morphFunc = [=](Model *model, cg3d::Visitor *visitor)
	{
		int cycle = currcycle % model->GetMesh()->data.size(); // curr cycle is number of times space pressed
		// modulo is so to get a valid number of mesh to choose -> 13 clicks -> mesh 3
		return cycle;
	};
	
	// model creation for automotphing
	sphere1 = Model::Create("sphere", sphereMesh, material);
	cyl = Model::Create("cyl", cylMesh, material);
	cube = Model::Create("cube", cubeMesh, material);

	auto autoSphere1 = AutoMorphingModel::Create(*sphere1, morphFunc);
	auto autoCube = AutoMorphingModel::Create(*cube, morphFunc);
	auto autoCyl = AutoMorphingModel::Create(*cyl, morphFunc);

	// object setup

	autoSphere1->Scale(2);
	autoSphere1->showWireframe = true;
	autoSphere1->Translate({-3, 0, 0});
	autoCyl->Translate({3, 0, 0});
	autoCyl->Scale(0.12f);
	autoCyl->showWireframe = true;
	autoCube->showWireframe = true;
	camera->Translate(20, Axis::Z);
	root->AddChild(autoSphere1);
	root->AddChild(autoCyl);
	root->AddChild(autoCube);

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
			currcycle++;
		if (key == GLFW_KEY_R)
			currcycle = 0;
	}
}