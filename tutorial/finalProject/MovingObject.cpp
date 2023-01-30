#include "MovingObject.h"
#include "Util.h"

#define SPEED 10
#define TIMEOUT 1000


Game::MovingObject::MovingObject(std::shared_ptr<cg3d::Material> material, std::shared_ptr<cg3d::Model> model, SnakeGame* scene) : Game::GameObject(material, model, scene)
{

    xCoordinate = model->GetTranslation()(0);
    yCoordinate = model->GetTranslation()(1);
    zCoordinate = model->GetTranslation()(2);
	speed = SPEED;
	this->moveBackwards = false;
    this->GenerateBezierCurve();
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
	std::random_device randDevice;
	std::mt19937       randGenerator(randDevice());
	std::uniform_int_distribution<int> distributeX(xCoordinate, xCoordinate + 4);
	std::uniform_int_distribution<int> distributeY(yCoordinate-1, yCoordinate + 5);
	std::uniform_int_distribution<int> distributeZ(zCoordinate-5, zCoordinate + 5 );

// 
	pRow1 = Eigen::Vector3f(distributeX(randGenerator), distributeY(randGenerator), distributeZ(randGenerator));
	pRow2 = Eigen::Vector3f(distributeX(randGenerator), distributeY(randGenerator), distributeZ(randGenerator));
	pRow3 = Eigen::Vector3f(distributeX(randGenerator), distributeY(randGenerator), distributeZ(randGenerator));
	pRow4 = Eigen::Vector3f(distributeX(randGenerator), distributeY(randGenerator), distributeZ(randGenerator));

	curvePoints.row(0) = pRow1;
	curvePoints.row(1) = pRow2;
	curvePoints.row(2) = pRow3;
	curvePoints.row(3) = pRow4;

	M << -1, 3, -3, 1,
		3, -6, 3, 0,
		-3, 3, 0, 0,
		1, 0, 0, 0;

	MG_Result = M * curvePoints;
	// DrawCurve(); 
}

Eigen::Vector3f Game::MovingObject::MoveBezier()
{
 	T[0] = powf(t, 3);
	T[1] = powf(t, 2);
	T[2] = t;
	T[3] = 1;

	currentPosition = (T * MG_Result);
	model->Translate(currentPosition);
	return(currentPosition);
}

void Game::MovingObject::Move()
{
	float min = 1;
	float max = 10;
	float distXY = Util::GenerateRandomInRange(min, max);
	float distZ = Util::GenerateRandomInRange(min, max);
	// TEMP
	Eigen::Vector3f moveVec = Eigen::Vector3f(distXY, distXY, distZ);
	Util::PrintVector(moveVec);
	model->Translate((moveVec).normalized()*0.0001*speed) ;
	
}

void Game::MovingObject::SetTimeOut(){
    isActive = false;
    timeout = TIMEOUT;
}



void Game::MovingObject::DrawCurve() {
	
	
}
