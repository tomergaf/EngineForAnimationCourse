#include "MovingObject.h"
#include "GameManager.h"
#include "Util.h"
#include "ViewerData.h"
#include "ViewerCore.h"



#define SPEED 10
#define DAMPENER 0.0007
#define TIMEOUT 1000


Game::MovingObject::MovingObject(std::shared_ptr<cg3d::Material> material, std::shared_ptr<cg3d::Model> model, SnakeGame* scene) : Game::GameObject(material, model, scene)
{

    xCoordinate = model->GetTranslation()(0);
    yCoordinate = model->GetTranslation()(1);
    zCoordinate = model->GetTranslation()(2);
	speed = SPEED;
	this->moveBackwards = false;
	this->t = 0;
	// this->moveVec = GenerateMoveVec();
	this->moveVec = GenerateBezierVec();
    // this->GenerateBezierCurve();
}

// Game::MovingObject* Game::MovingObject::SpawnObject(float xCoordinate, float yCoordinate, float zCoordinate, std::shared_ptr<cg3d::Material> material, std::shared_ptr<cg3d::Model> model, SnakeGame *scene)
// {
//     //create object
// 	Game::MovingObject* movingObject = new Game::MovingObject{material, model, scene};
// 	//move to location
// 	scene->root->AddChild(movingObject->model);
// 	Eigen::Vector3f posVec{xCoordinate, yCoordinate, zCoordinate};
// 	movingObject->model->Translate(posVec);
// 	//add to logs
// 	Util::DebugPrint(movingObject->name + " added at : " + std::to_string(xCoordinate) + ", " + std::to_string(yCoordinate) + ", " + std::to_string(zCoordinate));
// 	return movingObject;
// }

void Game::MovingObject::GenerateBezierCurve() {

	Eigen::Vector3f pRow1;
	Eigen::Vector3f pRow2;
	Eigen::Vector3f pRow3;
	Eigen::Vector3f pRow4;

	//bezier curves slides

	float minX = -3;
	float minY = -3;
	float minZ = -3;
	float maxX = 5;
	float maxY = 3;
	float maxZ = 5;

	// float randX = Util::GenerateRandomInRange(minX,maxX);
	// float randY = Util::GenerateRandomInRange(minY,maxY);
	// float randZ = Util::GenerateRandomInRange(minZ,maxZ);
	// std::uniform_int_distribution<int> distributeX(xCoordinate, xCoordinate + 4);
	// std::uniform_int_distribution<int> distributeY(yCoordinate-1, yCoordinate + 5);
	// std::uniform_int_distribution<int> distributeZ(zCoordinate-5, zCoordinate + 5 );


	pRow1 = Eigen::Vector3f(Util::GenerateRandomInRange(minX,maxX), Util::GenerateRandomInRange(minY,maxY), Util::GenerateRandomInRange(minZ,maxZ));
	pRow2 = Eigen::Vector3f(Util::GenerateRandomInRange(minX,maxX), Util::GenerateRandomInRange(minY,maxY), Util::GenerateRandomInRange(minZ,maxZ));
	pRow3 = Eigen::Vector3f(Util::GenerateRandomInRange(minX,maxX), Util::GenerateRandomInRange(minY,maxY), Util::GenerateRandomInRange(minZ,maxZ));
	pRow4 = Eigen::Vector3f(Util::GenerateRandomInRange(minX,maxX), Util::GenerateRandomInRange(minY,maxY), Util::GenerateRandomInRange(minZ,maxZ));

	curvePoints.row(0) = pRow1.cast<double>();
	curvePoints.row(1) = pRow2.cast<double>();
	curvePoints.row(2) = pRow3.cast<double>();
	curvePoints.row(3) = pRow4.cast<double>();

	M << -1, 3, -3, 1,
		3, -6, 3, 0,
		-3, 3, 0, 0,
		1, 0, 0, 0;

	MG = M * curvePoints;

	// DrawCurve(); 
}

Eigen::Vector3f Game::MovingObject::GenerateBezierVec()
{
	GenerateBezierCurve();

 	T[0] = powf(t, 3);
	T[1] = powf(t, 2);
	T[2] = t;
	T[3] = 1;

	currentPosition = (T * MG);
	// model->Translate(currentPosition.cast<float>());

	double dampener = DAMPENER;
	Eigen::Vector3f moveVec = currentPosition.cast<float>().normalized() * dampener * speed;
	this->moveVec = moveVec;
	return(moveVec);
}

Eigen::Vector3f Game::MovingObject::GenerateMoveVec()
{

	float min = -10;
	float max = 10;
	float distXY = Util::GenerateRandomInRange(min, max);
	float distZ = Util::GenerateRandomInRange(min, max);
	// TEMP
	double dampener = DAMPENER;
	Eigen::Vector3f moveVec = Eigen::Vector3f(distXY, distXY, distZ).normalized() * dampener * speed;
	
	this->moveVec = moveVec;
	// Util::PrintVector(moveVec);
    return moveVec;
}

void Game::MovingObject::Move()
{
	model->Translate(this->moveVec);
	
}

void Game::MovingObject::SetTimeOut(){
    isActive = false;
    timeout = TIMEOUT;
}



void Game::MovingObject::DrawCurve() {
	// Eigen::Vector3d drawingColor = Eigen::RowVector3d(1, 1, 1);
	Eigen::Matrix3d drawingColor = Eigen::Matrix3d::Ones();
	int meshIdx = index;
	Eigen::RowVector3d points; // Vertices of the mesh (#V x 3)
    Eigen::RowVector3d colors; // Faces of the mesh (#F x 3)
    Eigen::RowVector3d edges; // One normal per vertex    
	Eigen::MatrixXd verts ;
	Eigen::MatrixXi faces ;
	for (int i = 0; i < 3; i++){
		// this->scene->data(index).add_edges(curvePoints.row(i).cast<double>(), curvePoints.row(i + 1).cast<double>(), drawingColor);
		// this->scene->data(index).add_edges(curvePoints.row(i), curvePoints.row(i+1), drawingColor);
		verts = model->GetMeshList()[0]->data[0].vertices;
		verts.resize(verts.rows()+3, verts.cols());
		verts.row(verts.rows()-3+i) = curvePoints.row(i);
		// scene->data().add_edges(curvePoints.row(i), curvePoints.row(i+1), drawingColor);
		faces = model->GetMeshList()[0]->data[0].faces;
		faces.resize(faces.rows()+3, faces.cols());
		faces.row(faces.rows()-3+i) = curvePoints.row(i).cast<int>();
		// scene->data().add_edges(curvePoints.row(i), curvePoints.row(i+1), drawingColor);
		// Util::PrintVector(curvePoints.row(i).cast<float>());
		// this->scene->data().add_edges(curvePoints.row(i), curvePoints.row(i + 1), drawingColor);
		// model->AddOverlay(data, false);
		// points = curvePoints.row(i), curvePoints.row(i+1), drawingColor);
		
	}
	// model->showWireframe = true;
	faces = model->GetMeshList()[0]->data[0].faces;
	Eigen::MatrixXd VN;
	igl::per_vertex_normals(verts, faces, VN);
	Eigen::MatrixXd TC = Eigen::MatrixXd::Zero(verts.rows(), 2);
	// model->GetMeshList()[0]->data.emplace(model->GetMeshList()[0]->data.begin(), cg3d::MeshData{verts, faces, VN, TC });
	model->GetMeshList()[0]->data.push_back(cg3d::MeshData{verts, faces, VN, TC });
	// for (int i = 0; i < 3; i++){
	// 	// this->scene->data(index).add_edges(curvePoints.row(i).cast<double>(), curvePoints.row(i + 1).cast<double>(), drawingColor);
	// 	// this->scene->data(index).add_edges(curvePoints.row(i), curvePoints.row(i+1), drawingColor);
	// 	cg3d::OverlayData data{curvePoints.row(i), drawingColor, curvePoints.row(i+1)};
	// 	Util::PrintVector(curvePoints.row(i).cast<float>());
	// 	model->AddOverlay(data, true);
	// 	// points = curvePoints.row(i), curvePoints.row(i+1), drawingColor);
		
	// 	// this->scene->data(index).add_edges(curvePoints.row(i), curvePoints.row(i + 1), drawingColor);
	// }
		// model->mode =1;

	scene->data().line_width = 25;
	scene->data().show_lines = true;
	scene->data().show_overlay_depth = true;
}
	
	

