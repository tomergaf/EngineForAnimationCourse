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
#include <igl/AABB.h>

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
	// scene setup
	i_fov = fov;
	i_width = width;
	i_height = height;
	i_near = near;
	i_far = far;
    camera = Camera::Create("camera", fov, float(width) / float(height), near, far);
	auto program = std::make_shared<Program>("shaders/phongShader");
	program->name = "phong";
	auto cube_program = std::make_shared<Program>("shaders/basicShader1");
	auto material = std::make_shared<Material>("material", program); // empty material
	auto cube_material = std::make_shared<Material>("material", program); // empty material
	auto daylight{std::make_shared<Material>("daylight", "shaders/cubemapShader")};
	daylight->AddTexture(0, "textures/cubemaps/Daylight Box_", 3);
	auto background{Model::Create("background", Mesh::Cube(), daylight)};

	AddChild(root = Movable::Create("root")); // a common (invisible) parent object for all the shapes

	AddChild(background);
	background->Scale(120, Axis::XYZ);
	background->SetPickable(false);
	background->SetStatic();

	material->AddTexture(0, "textures/bricks.jpg", 2);
	cube_material->AddTexture(0, "textures/bricks.jpg", 2);

	auto staticBunnyMesh{IglLoader::MeshFromFiles("bunny_static", "data/bunny.off")};
	auto movingBunnyMesh{IglLoader::MeshFromFiles("bunny_moving", "data/bunny.off")};
	auto collisionCube1Mesh {IglLoader::MeshFromFiles("collisioncube1mesh", "data/cube.off")};
	auto collisionCube2Mesh {IglLoader::MeshFromFiles("collisioncube2mesh", "data/cube.off")};

	decimationMult = 0.5; // the modifier to decimate with - change this to increase / decrease edge decimation

	// pre decimate mesh so user can cycle at runtime - change to false to run default decimation
	bool customPredecimate = false;

    PreDecimateMesh(staticBunnyMesh, customPredecimate); //pre decimation for ease of calculation
	PreDecimateMesh(movingBunnyMesh, customPredecimate);

	// init vars for collision
	velocityX = 0.0004; //set moving velocity horizontal
	velIntervalX = 0.001; //velocity interval for increase/decrease on input button
	velocityY = 0; //set moving velocity vertical
	velIntervalY = 0.001;
	
	objectsCollided=false; // init collision bool
    
	// model creation
    staticBunny = Model::Create("staticBunny", staticBunnyMesh, material);
    movingBunny = Model::Create("movingBunny", movingBunnyMesh, material);
	
	//collision cube creation
    collisionCube1 = Model::Create("collisioncube1", collisionCube1Mesh, cube_material);
    collisionCube2 = Model::Create("collisioncube2", collisionCube2Mesh, cube_material);

	staticBunny->AddChild(collisionCube1); //add collision cube as child so it moves and rotates with parent
	movingBunny->AddChild(collisionCube2); 
    
    // object setup
	camera->Translate(10, Axis::Z);
	camera->Translate(0.5, Axis::Y);
	staticBunny->showWireframe = false;
	movingBunny->showWireframe = false;
	
	collisionCube1->showFaces = false;
    collisionCube2->showFaces = false;
    collisionCube1->showWireframe = true;
    collisionCube2->showWireframe = true;
	
	staticBunny->Translate({-0.5, 0, 8});
	movingBunny->Translate({0.5, 0, 8});
	clickMap.insert({staticBunny.get(), 0}); // load predecimated mesh
	clickMap.insert({movingBunny.get(), 0}); // load predecimated mesh

	root->AddChild(staticBunny);
	root->AddChild(movingBunny);

	// allign and draw boxes
	auto mesh = staticBunny->GetMeshList();
	igl::AABB<Eigen::MatrixXd, 3> objectTree;
    Ve.push_back(mesh[0]->data[0].vertices); // push vertices of collapsed mesh
    Fa.push_back(mesh[0]->data[0].faces);
    boxTree1.init(Ve[0], Fa[0]);
	boxTrees.push_back(boxTree1);

    mesh = movingBunny->GetMeshList();
    Ve.push_back(mesh[0]->data[0].vertices); // // push vertices of collapsed mesh
    Fa.push_back(mesh[0]->data[0].faces);
    boxTree2.init(Ve[1], Fa[1]);
	boxTrees.push_back(boxTree2);

    DrawBox(boxTree1.m_box, 1, collisionCube1); // draw the collision boxes outline
    DrawBox(boxTree2.m_box, 1, collisionCube2);
}

void CollisionScene::DrawBox(Eigen::AlignedBox<double, 3>& box, int color,std::shared_ptr<cg3d::Model> model){
	Eigen::RowVector3d colorVector;
	if (color == 1) {
		// model->material->program->SetUniform4f("lightColor", 0.0f, 0.8f,0.0f, 0.5f);
		model->material->AddTexture(0, "textures/grass.bmp", 2);
		//model->material->BindTextures();
	}
	else if (color == 2) {
		// model->material->program->SetUniform4f("lightColor", 0.8f, 0.0f,0.0f, 0.5f);
		model->material->AddTexture(0, "textures/box0.bmp", 2);
		// model->material->BindTextures();
	}
		
	Eigen::RowVector3d BottomRightCeil = box.corner(box.BottomRightCeil);
	Eigen::RowVector3d BottomRightFloor = box.corner(box.BottomRightFloor);
	Eigen::RowVector3d BottomLeftCeil = box.corner(box.BottomLeftCeil);
	Eigen::RowVector3d BottomLeftFloor = box.corner(box.BottomLeftFloor);
	Eigen::RowVector3d TopRightCeil = box.corner(box.TopRightCeil);
	Eigen::RowVector3d TopRightFloor = box.corner(box.TopRightFloor);
	Eigen::RowVector3d TopLeftCeil = box.corner(box.TopLeftCeil);
	Eigen::RowVector3d TopLeftFloor = box.corner(box.TopLeftFloor);
	Eigen::MatrixXd V, VN, T;
    Eigen::MatrixXi F;

    F.resize(12, 3); // 3d cubes have 12 faces 
    V.resize(8, 3); // and 8 vertices
    
	#pragma region cubemap
	// this maps the cube coords to vertices and faces to vertices
	V.row(0) = BottomLeftFloor;
    V.row(1) = BottomRightFloor;
    V.row(2) = TopLeftFloor;
    V.row(3) = TopRightFloor;
    V.row(4) = BottomLeftCeil;
    V.row(5) = BottomRightCeil;
    V.row(6) = TopLeftCeil;
    V.row(7) = TopRightCeil;
    F.row(0) = Eigen::Vector3i(0, 1, 3);
    F.row(1) = Eigen::Vector3i(3, 2, 0);
    F.row(2) = Eigen::Vector3i(4, 5, 7);
    F.row(3) = Eigen::Vector3i(7, 6, 4);
    F.row(4) = Eigen::Vector3i(0, 4, 6);
    F.row(5) = Eigen::Vector3i(6, 2, 0);
    F.row(6) = Eigen::Vector3i(5, 7, 3);
    F.row(7) = Eigen::Vector3i(7, 3, 1);
    F.row(8) = Eigen::Vector3i(2, 6, 7);
    F.row(9) = Eigen::Vector3i(7, 3, 2);
    F.row(10) = Eigen::Vector3i(4, 5, 1);
    F.row(11) = Eigen::Vector3i(1, 0, 4);

	#pragma endregion

    igl::per_vertex_normals(V, F, VN);
    T = Eigen::MatrixXd::Zero(V.rows(), 2);

    auto mesh = model->GetMeshList();
    mesh[0]->data.push_back({ V, F, VN, T }); // push new cube mesh to draw
    model->SetMeshList(mesh);
    model->meshIndex = mesh[0]->data.size()-1; // use last mesh pushed
}

bool CollisionScene::ObjectsCollided(igl::AABB<Eigen::MatrixXd, 3>* boxTreeA, igl::AABB<Eigen::MatrixXd, 3>* boxTreeB, std::shared_ptr<cg3d::Model> model, std::shared_ptr<cg3d::Model> otherModel){
	if (boxTreeA == nullptr || boxTreeB == nullptr){ // if no boxes at all
		return false;
	}
	
	if (!BoxesIntersect(boxTreeA->m_box, boxTreeB->m_box, model, otherModel)) { //if boxes of the two models dont intersect
		return false;
	}
	if (boxTreeA->is_leaf() && boxTreeB->is_leaf()) {
		//if the two boxes are leaves and they intersect, draw them
		if(BoxesIntersect(boxTreeA->m_box, boxTreeB->m_box, model, otherModel)){
			DrawBox(boxTreeA->m_box, 2, collisionCube1);
			DrawBox(boxTreeB->m_box,2, collisionCube2);
			debug_print("collision");
			objectsCollided = true;
			return true;
		}
		else { //else leaves do not intersect
			return false;
		}

	}
	 if (boxTreeA->is_leaf() && !boxTreeB->is_leaf()) {
		 return ObjectsCollided(boxTreeA, boxTreeB-> m_right, model, otherModel) ||
			 ObjectsCollided(boxTreeA, boxTreeB->m_left, model, otherModel);
	}
	 if (!boxTreeA->is_leaf() && boxTreeB->is_leaf()) {
		 return ObjectsCollided(boxTreeA->m_right, boxTreeB, model, otherModel) ||
			  ObjectsCollided(boxTreeA->m_left, boxTreeB, model, otherModel);
	 }

	// check recursive conditions
    return ObjectsCollided(boxTreeA->m_left, boxTreeB->m_left, model, otherModel) ||
		   ObjectsCollided(boxTreeA->m_left, boxTreeB->m_right, model, otherModel) ||
		   ObjectsCollided(boxTreeA->m_right, boxTreeB->m_left, model, otherModel) ||
		   ObjectsCollided(boxTreeA->m_right, boxTreeB->m_right, model, otherModel);
}

bool CollisionScene::BoxesIntersect(Eigen::AlignedBox<double, 3>& boxA, Eigen::AlignedBox<double, 3>& boxB , std::shared_ptr<cg3d::Model> model, std::shared_ptr<cg3d::Model> otherModel){
	// check matrices overlap as described in slides
	
	// matrix A
	Eigen::Matrix3d A = model->GetRotation().cast<double>();
	Eigen::Vector3d A0 = A.col(0);
	Eigen::Vector3d A1 = A.col(1);
	Eigen::Vector3d A2 = A.col(2);

	// matrix B
	Eigen::Matrix3d B = otherModel->GetRotation().cast<double>();
	Eigen::Vector3d B0 = B.col(0);
	Eigen::Vector3d B1 = B.col(1);
	Eigen::Vector3d B2 = B.col(2);
	
	// C=A^T*B
	Eigen::Matrix3d C = A.transpose() * B;
	
	// aabb edge length is size/2
	Eigen::Vector3d a = boxA.sizes();
	Eigen::Vector3d b = boxB.sizes();
	a = a / 2;
	b = b / 2;
	// matrix D
	Eigen::Vector4d CenterA = Eigen::Vector4d(boxA.center()[0], boxA.center()[1], boxA.center()[2], 1);
	Eigen::Vector4d CenterB = Eigen::Vector4d(boxB.center()[0], boxB.center()[1], boxB.center()[2], 1);
	Eigen::Vector4d D4d = otherModel->GetTransform().cast<double>() * CenterB - model->GetTransform().cast<double>() * CenterA;
	Eigen::Vector3d D = D4d.head(3);
	
	// A conditions
	if (a(0) + (b(0) * abs(C.row(0)(0)) + b(1) * abs(C.row(0)(1)) + b(2) * abs(C.row(0)(2))) < abs(A0.transpose() * D))
	// if (a(0) + b.dot(C.row(0)) < abs(A0.transpose() * D))
		return false;
	if (a(1) + (b(0) * abs(C.row(1)(0)) + b(1) * abs(C.row(1)(1)) + b(2) * abs(C.row(1)(2))) < abs(A1.transpose() * D))
	// if (a(1) + b.dot(C.row(1)) < abs(A1.transpose() * D))
		return false;
	if (a(2) + (b(0) * abs(C.row(2)(0)) + b(1) * abs(C.row(2)(1)) + b(2) * abs(C.row(2)(2))) < abs(A2.transpose() * D))
	// if (a(2) + b.dot(C.row(2)) < abs(A2.transpose() * D))
		return false;
	// B conditions
	if (b(0) + (a(0) * abs(C.row(0)(0)) + a(1) * abs(C.row(1)(0)) + a(2) * abs(C.row(2)(0))) < abs(B0.transpose() * D))
	// if (b(0) + a.dot(C.row(0)) < abs(B0.transpose() * D))
		return false;
	if (b(1) + (a(0) * abs(C.row(0)(1)) + a(1) * abs(C.row(1)(1)) + a(2) * abs(C.row(2)(1))) < abs(B1.transpose() * D))
	// if (b(1) + a.dot(C.row(1)) < abs(B1.transpose() * D))
		return false;
	if (b(2) + (a(0) * abs(C.row(0)(2)) + a(1) * abs(C.row(1)(2)) + a(2) * abs(C.row(2)(2))) < abs(B2.transpose() * D))
	// if (b(2) + a.dot(C.row(2)) < abs(B2.transpose() * D))
		return false;
	// A0 conditions
	double R = C.row(1)(0) * A2.transpose() * D;
	R-=C.row(2)(0) * A1.transpose() * D;
	if (a(1) * abs(C.row(2)(0)) + a(2) * abs(C.row(1)(0)) + b(1) * abs(C.row(0)(2))+ b(2) * abs(C.row(0)(1)) < abs(R))
		return false;

	R = C.row(1)(1) * A2.transpose() * D;
	R -= C.row(2)(1) * A1.transpose() * D;
	if (a(1) * abs(C.row(2)(1)) + a(2) * abs(C.row(1)(1)) + b(0) * abs(C.row(0)(2)) + b(2) * abs(C.row(0)(0)) < abs(R))
		return false;

	R = C.row(1)(2) * A2.transpose() * D;
	R -= C.row(2)(2) * A1.transpose() * D;
	if (a(1) * abs(C.row(2)(2)) + a(2) * abs(C.row(1)(2)) + b(0) * abs(C.row(0)(1)) + b(1) * abs(C.row(0)(0)) < abs(R))
		return false;
	// A1 conditions

	R = C.row(2)(0) * A0.transpose() * D;
	R -= C.row(0)(0) * A2.transpose() * D;
	if (a(0) * abs(C.row(2)(0)) + a(2) * abs(C.row(0)(0)) + b(1) * abs(C.row(1)(2)) + b(2) * abs(C.row(1)(1)) < abs(R))
		return false;

	R = C.row(2)(1) * A0.transpose() * D;
	R -= C.row(0)(1) * A2.transpose() * D;
	if (a(0) * abs(C.row(2)(1)) + a(2) * abs(C.row(0)(1)) + b(0) * abs(C.row(1)(2)) + b(2) * abs(C.row(1)(0)) < abs(R))
		return false;

	R = C.row(2)(2) * A0.transpose() * D;
	R -= C.row(0)(2) * A2.transpose() * D;
	if (a(0) * abs(C.row(2)(2)) + a(2) * abs(C.row(0)(2)) + b(0) * abs(C.row(1)(1)) + b(1) * abs(C.row(1)(0)) < abs(R))
		return false;
	// A2 conditions

	R = C.row(0)(0) * A1.transpose() * D;
	R -= C.row(1)(0) * A0.transpose() * D;
	if (a(0) * abs(C.row(1)(0)) + a(1) * abs(C.row(0)(0)) + b(1) * abs(C.row(2)(2)) + b(2) * abs(C.row(2)(1)) < abs(R))
		return false;

	R = C.row(0)(1) * A1.transpose() * D;
	R -= C.row(1)(1) * A0.transpose() * D;
	if (a(0) * abs(C.row(1)(1)) + a(1) * abs(C.row(0)(1)) + b(0) * abs(C.row(2)(2)) + b(2) * abs(C.row(2)(0)) < abs(R))
		return false;
	R = C.row(0)(2) * A1.transpose() * D;
	R -= C.row(1)(2) * A0.transpose() * D;
	if (a(0) * abs(C.row(1)(2)) + a(1) * abs(C.row(0)(2)) + b(0) * abs(C.row(2)(1)) + b(1) * abs(C.row(2)(0)) < abs(R))
		return false;
	
	return true;

}

void CollisionScene::AnimateUntilCollision(std::shared_ptr<cg3d::Model> model, float velocity){
	if(!objectsCollided){
		Eigen::Vector3f moveVector = {-1*velocityX,1*velocityY,0};
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

void CollisionScene::UpdateXVelocity(bool direction){ //function for changing velocity on button click

	float change = direction ? velIntervalX*1 : velIntervalX*-1;
	velocityX += change;
}

void CollisionScene::UpdateYVelocity(bool direction){
	float change = direction ? velIntervalY*1 : velIntervalY*-1;
	velocityY += change;
}

void CollisionScene::StopMotion(){
	velocityX = velocityY = 0;
}

void CollisionScene::Update(const Program &program, const Eigen::Matrix4f &proj, const Eigen::Matrix4f &view, const Eigen::Matrix4f &model)
{
	Scene::Update(program, proj, view, model);
	#pragma region lighting
	if(program.name=="phong"){
		
		program.SetUniform4f("lightColor", 0.8f, 0.8f,0.8f, 0.5f);
		program.SetUniform4f("Kai", 1.0f, 0.3f, 0.6f, 1.0f);
		program.SetUniform4f("Kdi", 0.5f, 0.5f, 0.7f, 1.0f);
		program.SetUniform4f("Ksi", 0.7f, 0.7f, 0.7f, 1.0f);
		program.SetUniform1f("specular_exponent", 5.0);
		program.SetUniform4f("light_position", 0.0, 15.0, -3.0, 1.0);
	}

	#pragma endregion

	// ObjectsCollided(&boxTrees[0],&boxTrees[1], staticBunny, movingBunny);
	if (!objectsCollided){
		ObjectsCollided(&boxTree1, &boxTree2, staticBunny, movingBunny);
		// ObjectsCollided(&boxTrees[0], &boxTrees[1], staticBunny, movingBunny);
		AnimateUntilCollision(movingBunny, velocityX);
	}
}

void CollisionScene::KeyCallback(Viewport *_viewport, int x, int y, int key, int scancode, int action, int mods)
{
	
	if (action == GLFW_PRESS || action == GLFW_REPEAT)
	{
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
		if (key == GLFW_KEY_R) 
			Init(i_fov, i_width, i_height, i_near, i_far);
	}
}