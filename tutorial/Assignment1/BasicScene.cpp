#include "BasicScene.h"
#include <read_triangle_mesh.h>
#include <utility>
#include "ObjLoader.h"
#include "IglMeshLoader.h"
#include "igl/read_triangle_mesh.cpp"
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

	std::vector<MeshData> sphereMeshVector;
	std::vector<std::shared_ptr<Mesh>> sphereMeshmVector;
	std::vector<MeshData> cylMeshVector;
	std::vector<MeshData> cubeMeshVector;

	cylMeshVector.push_back(cylMesh->data[0]);
	sphereMeshVector.push_back(sphereMesh->data[0]);
	cubeMeshVector.push_back(cubeMesh->data[0]);

	MatrixXd V, OV;
	MatrixXi F, OF;
	// read_triangle_mesh("data/camel_b.obj", OV, OF);
	// OV = cubeMeshVector.at(0).vertices;
	// OF = cubeMeshVector.at(0).faces;
	OV = sphereMeshVector.at(0).vertices;
	OF = sphereMeshVector.at(0).faces;
	// OV = cylMeshVector.at(0).vertices;
	// OF = cylMeshVector.at(0).faces;

	Eigen::VectorXi EMAP;
	Eigen::MatrixXi E, EF, EI;
	Eigen::VectorXi EQ;
	igl::min_heap<std::tuple<double, int, int>> Q;

	MatrixXd C;
	int num_collapsed;

	// RESET
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
	// one cut attempt

	auto cutedges = [&]() mutable
	{
		if (!Q.empty())
		{
			// collapse edge
			const int max_iter = std::ceil(0.06 * Q.size());
			for (int j = 0; j < max_iter; j++)
			{
				if (!collapse_edge(shortest_edge_and_midpoint, V, F, E, EMAP, EF, EI, Q, EQ, C))
				{
					break;
				}
				num_collapsed++;
			}
		//sphereMeshmVector.push_back(make_shared<Mesh>("cut", V, F, sphereMesh->data[0].vertexNormals, sphereMesh->data[0].textureCoords));
		//sphereMesh->data.push_back(make_shared<MeshData>{ V, F});
		sphereMesh->data.push_back(MeshData{V, F});
		}
	};
	reset();
	cutedges();
	cutedges();
	cutedges();

	// test sort this later and makes cycle change by clicking space

	/*
	STOPPED HERE:
	- need to use a automorphingmodel to switch between meshdatas
	- need to connect a callback for clicking space and increasing cycle count
	- need to link picker to model mesh to cycle
	- need to tidy this up in general and have a good chronological order
	- need to check how many edges are being removed
	- still need to make another collapse edge function as requested
	
	*/

	int currcycle=2;
	auto blank{std::make_shared<Material>("blank", program)};
    auto test{Model::Create("test", sphereMesh, blank)};

	auto morphFunc = [&](int currcycle) {
        float cycle = sphereMesh->data.size()%currcycle ;
        
        return cycle;
    };
    auto sphere2 = AutoMorphingModel::Create(*test, morphFunc);

	//auto cutmesh = make_shared<Mesh>("cut", V, F, sphereMesh->data[0].vertexNormals, sphereMesh->data[0].textureCoords);
	/* sphereMeshmVector.push_back(cutmesh);
	cutedges;
	cutmesh = make_shared<Mesh>("cut", V, F, sphereMesh->data[0].vertexNormals, sphereMesh->data[0].textureCoords);
	 *///sphereMeshmVector.push_back(cutmesh);
	//cutedges;
	//cutmesh = make_shared<Mesh>("cut", V, F, sphereMesh->data[0].vertexNormals, sphereMesh->data[0].textureCoords);
	//sphereMeshmVector.push_back(cutmesh);
	// auto cutmesh = make_shared<Mesh>("cut", V, F, cylMesh->data[0].vertexNormals, cylMesh->data[0].textureCoords);
	// auto cutmesh = make_shared<Mesh>("cut", V, F, cubeMesh->data[0].vertexNormals, cubeMesh->data[0].textureCoords);

	// change these to get mesh from automorphingmodel
	//sphere2 = Model::Create("sphere_cut", sphereMeshmVector.at(1), material);
	sphere1 = Model::Create("sphere", sphereMesh, material);
	//sphere2 = Model::Create("sphere_cut", sphereMeshmVector.at(0), material);
	//sphere3 = Model::Create("sphere_cut", sphereMeshmVector.at(1), material);
	//sphere4 = Model::Create("sphere_cut", sphereMeshmVector.at(2), material);
	
	cyl = Model::Create("cyl", cylMesh, material);
	cube = Model::Create("cube", cubeMesh, material);
	sphere1->Scale(2);
	sphere1->showWireframe = true;
	sphere1->Translate({-3, 0, 0});
	sphere2->Scale(2);
	sphere2->showWireframe = true;
	sphere2->Translate({-6, 0, 0});
	sphere3->Scale(2);
	sphere3->showWireframe = true;
	sphere3->Translate({-9, 0, 0});
	sphere4->Scale(2);
	sphere4->showWireframe = true;
	sphere4->Translate({-12, 0, 0});
	cyl->Translate({3, 0, 0});
	cyl->Scale(0.12f);
	cyl->showWireframe = true;
	cube->showWireframe = true;
	camera->Translate(20, Axis::Z);
	root->AddChild(sphere1);
	root->AddChild(sphere2);
	root->AddChild(sphere3);
	root->AddChild(sphere4);
	root->AddChild(cyl);
	root->AddChild(cube);

}

void BasicScene::Update(const Program &program, const Eigen::Matrix4f &proj, const Eigen::Matrix4f &view, const Eigen::Matrix4f &model)
{
	Scene::Update(program, proj, view, model);

	cube->Rotate(0.01f, Axis::XYZ);
}
