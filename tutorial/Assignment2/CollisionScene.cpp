#include "CollisionScene.h"
#include <read_triangle_mesh.h>
#include <utility>
#include "ObjLoader.h"
#include "IglMeshLoader.h"
#include "igl/read_triangle_mesh.cpp"
#include "igl/per_vertex_normals.h"
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

void CollisionScene::Init(float fov, int width, int height, float near, float far)
{
    camera = Camera::Create("camera", fov, float(width) / float(height), near, far);
	auto program = std::make_shared<Program>("shaders/phongShader");
	auto material = std::make_shared<Material>("material", program); // empty material
	auto daylight{std::make_shared<Material>("daylight", "shaders/cubemapShader")};
	daylight->AddTexture(0, "textures/cubemaps/Daylight Box_", 3);
	auto background{Model::Create("background", Mesh::Cube(), daylight)};

	AddChild(root = Movable::Create("root")); // a common (invisible) parent object for all the shapes

	AddChild(background);
	background->Scale(120, Axis::XYZ);
	background->SetPickable(false);
	background->SetStatic();

	// material->AddTexture(0, "textures/box0.bmp", 2);
	material->AddTexture(0, "textures/bricks.jpg", 2);
	// auto sphereMesh{IglLoader::MeshFromFiles("sphere_igl", "data/sphere.obj")};
	// auto cowMesh{IglLoader::MeshFromFiles("cow", "data/cow.off")};
	auto staticBunnyMesh{IglLoader::MeshFromFiles("bunny_static", "data/bunny.off")};
	auto movingBunnyMesh{IglLoader::MeshFromFiles("bunny_moving", "data/bunny.off")};

	// morphing function - cycles through mesh indices by comparing to times pressed space
	auto morphFunc = [=](Model *model, cg3d::Visitor *visitor)
	{
		int cycle = clickMap[model] % model->GetMesh()->data.size(); // curr cycle is number of times space pressed
		// int cycle = model->GetMesh()->data.size(); // curr cycle is number of times space pressed
		// modulo is so to get a valid number of mesh to choose -> 13 clicks -> mesh 3
		debug_print(cycle);
		return cycle;
	};

	decimationMult = 0.5; // the modifier to decimate with - change this to increase / decrease edge decimation

	// pre decimate mesh so user can cycle at runtime - change to false to run default decimation
	bool customPredecimate = false;

    PreDecimateMesh(staticBunnyMesh, customPredecimate);
	PreDecimateMesh(movingBunnyMesh, customPredecimate);

    // model creation
    staticBunny = Model::Create("staticBunny", staticBunnyMesh, material);
    movingBunny = Model::Create("movingBunny", movingBunnyMesh, material);

    //auto morphing model creation
    // auto autoStaticBunny = AutoMorphingModel::Create(*staticBunny, morphFunc);
    // auto autoMovingBunny = AutoMorphingModel::Create(*movingBunny, morphFunc);

    // object setup
	camera->Translate(30, Axis::Z);
	camera->Translate(5, Axis::Y);
	staticBunny->showWireframe = false;
	movingBunny->showWireframe = false;
	staticBunny->Scale(20);
	movingBunny->Scale(20);
	staticBunny->Translate({-2, 0, 0});
	movingBunny->Translate({7, 0, 0});
	// autoStaticBunny->showWireframe = false;
	// autoMovingBunny->showWireframe = false;
	// autoStaticBunny->Scale(20);
	// autoMovingBunny->Scale(20);
	// autoStaticBunny->Translate({-2, 6, 0});
	// autoMovingBunny->Translate({5, 0, 0});
	
	clickMap.insert({staticBunny.get(), 0}); // load predecimated mesh
	clickMap.insert({movingBunny.get(), 0}); // load predecimated mesh
	root->AddChild(staticBunny);
	root->AddChild(movingBunny);
	// root->AddChild(autoStaticBunny);
	// root->AddChild(autoMovingBunny);

	velocity = 0.2; //set moving velocity
	objectsCollided=false;
}

void CollisionScene::AnimateUntilCollision(std::shared_ptr<cg3d::Model> model, float velocity){
	if(!objectsCollided){
		Eigen::Vector3f moveVector = {-1*velocity,0,0};
		model->Translate(moveVector);
	}
	
}

void CollisionScene::PreDecimateMesh(std::shared_ptr<cg3d::Mesh> mesh, bool custom)
{
	MatrixXd V, OV, VN, FN; // matrices for data
	MatrixXi F, OF;
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

	MatrixXd C;		   // cost matrices
	int num_collapsed; // counter of collapsed edges

#pragma region customCollapse
	auto computeQ = [&]() 
	{
		// this is used to initialize the Q matrices of each vertex 
		std::vector<std::vector<int>> V2F; // vertex to face vector
		std::vector<std::vector<int>> NI;  // for adj

		int n =V.rows();
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
	int times = 3;
	// the actual flow - first reset vals, then cut edges 10 times
	reset(custom);
	for (int i = 0; i < times; i++)
	{
		cutedges(custom);
	}
}

void CollisionScene::UpdateModelClickCount(bool direction)
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

void CollisionScene::Update(const Program &program, const Eigen::Matrix4f &proj, const Eigen::Matrix4f &view, const Eigen::Matrix4f &model)
{
	Scene::Update(program, proj, view, model);
	program.SetUniform4f("lightColor", 0.8f, 0.8f,0.8f, 0.5f);
    program.SetUniform4f("Kai", 1.0f, 0.3f, 0.6f, 1.0f);
    program.SetUniform4f("Kdi", 0.5f, 0.5f, 0.7f, 1.0f);
    program.SetUniform4f("Ksi", 0.7f, 0.7f, 0.7f, 1.0f);
    program.SetUniform1f("specular_exponent", 5.0);
    program.SetUniform4f("light_position", 0.0, 15.0, -3.0, 1.0);
	AnimateUntilCollision(movingBunny, velocity);
}

void CollisionScene::KeyCallback(Viewport *_viewport, int x, int y, int key, int scancode, int action, int mods)
{
	// TODO make this aware of picked object
	if (action == GLFW_PRESS || action == GLFW_REPEAT)
	{
		if (key == GLFW_KEY_SPACE)
			UpdateModelClickCount(true);
		if (key == GLFW_KEY_UP) //TODO change this to velocityup
			UpdateModelClickCount(true);
		if (key == GLFW_KEY_DOWN) //TODO change this to velocitydown
			UpdateModelClickCount(false);
		if (key == GLFW_KEY_R)
		{
			// Init();
			// reset all click counts
			for (auto &key : clickMap)
			{
				key.second = 0;
			}
		}
	}
}