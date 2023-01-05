#include "BasicScene.h"
#include <Eigen/src/Core/Matrix.h>
#include <edges.h>
#include <memory>
#include <per_face_normals.h>
#include <read_triangle_mesh.h>
#include <utility>
#include <vector>
#include "GLFW/glfw3.h"
#include "Mesh.h"
#include "PickVisitor.h"
#include "Renderer.h"
#include "ObjLoader.h"
#include "IglMeshLoader.h"

#include "igl/per_vertex_normals.h"
#include "igl/per_face_normals.h"
#include "igl/unproject_onto_mesh.h"
#include "igl/edge_flaps.h"
#include "igl/loop.h"
#include "igl/upsample.h"
#include "igl/AABB.h"
#include "igl/parallel_for.h"
#include "igl/shortest_edge_and_midpoint.h"
#include "igl/circulation.h"
#include "igl/edge_midpoints.h"
#include "igl/collapse_edge.h"
#include "igl/edge_collapse_is_valid.h"
#include "igl/write_triangle_mesh.h"

#include "Utility.h"
#define M_PI       3.14159265358979323846 
// #include "AutoMorphingModel.h"

using namespace cg3d;

void BasicScene::Init(float fov, int width, int height, float near, float far)
{
    shouldAnimateCCD = false;
    camera = Camera::Create( "camera", fov, float(width) / height, near, far);
    AddChild(root = Movable::Create("root")); // a common (invisible) parent object for all the shapes
    auto daylight{std::make_shared<Material>("daylight", "shaders/cubemapShader")}; 
    daylight->AddTexture(0, "textures/cubemaps/Daylight Box_", 3);
    auto background{Model::Create("background", Mesh::Cube(), daylight)};
    AddChild(background);
    background->Scale(120, Axis::XYZ);
    background->SetPickable(false);
    background->SetStatic();

 
    auto program = std::make_shared<Program>("shaders/phongShader");
    auto program1 = std::make_shared<Program>("shaders/pickingShader");
    
    auto material{ std::make_shared<Material>("material", program)}; // empty material
    auto material1{ std::make_shared<Material>("material", program1)}; // empty material
//    SetNamedObject(cube, Model::Create, Mesh::Cube(), material, shared_from_this());
 
    material->AddTexture(0, "textures/box0.bmp", 2);
    auto sphereMesh{IglLoader::MeshFromFiles("sphere_igl", "data/sphere.obj")};
    auto cylMesh{IglLoader::MeshFromFiles("cyl_igl","data/xcylinder.obj")};
    auto cubeMesh{IglLoader::MeshFromFiles("cube_igl","data/cube_old.obj")};
    sphere1 = Model::Create( "sphere",sphereMesh, material);    
    cube = Model::Create( "cube", cubeMesh, material);
    
    //Axis
    Eigen::MatrixXd vertices(6,3);
    vertices << -1,0,0,1,0,0,0,-1,0,0,1,0,0,0,-1,0,0,1;
    Eigen::MatrixXi faces(3,2);
    faces << 0,1,2,3,4,5;
    Eigen::MatrixXd vertexNormals = Eigen::MatrixXd::Ones(6,3);
    Eigen::MatrixXd textureCoords = Eigen::MatrixXd::Ones(6,2);
    std::shared_ptr<Mesh> coordsys = std::make_shared<Mesh>("coordsys",vertices,faces,vertexNormals,textureCoords);
    axis.push_back(Model::Create("axis",coordsys,material1));
    axis[0]->mode = 1;   
    axis[0]->Scale(4,Axis::XYZ);
    // axis[0]->lineWidth = 5;
    root->AddChild(axis[0]);
    float scaleFactor = 1; 
    cyls.push_back( Model::Create("cyl",cylMesh, material));
    cyls[0]->Scale(scaleFactor,Axis::X);
    // cyls[0]->SetCenter(Eigen::Vector3f(0,0,-0.8f*scaleFactor));
    cyls[0]->SetCenter(Eigen::Vector3f(-0.8f*scaleFactor,0,0));
    // cyls[0]->RotateByDegree(90, Eigen::Vector3f(0,1,0));
    root->AddChild(cyls[0]);
   
    for(int i = 1;i < 3; i++)
    { 
        //cyls
        cyls.push_back( Model::Create("cyl", cylMesh, material));
        cyls[i]->Scale(scaleFactor,Axis::X);   
        // cyls[i]->Translate(1.6f*scaleFactor,Axis::Z);
        cyls[i]->Translate(1.6f*scaleFactor,Axis::X);
        cyls[i]->SetCenter(Eigen::Vector3f(-0.8f*scaleFactor,0,0));
        // cyls[i]->SetCenter(Eigen::Vector3f(0,0,-0.8f*scaleFactor));
        cyls[i-1]->AddChild(cyls[i]);
         
        //axis drawing
        axis.push_back(Model::Create("axis", coordsys, material1));
        axis[i]->mode = 1;
        axis[i]->Scale(4, Axis::XYZ);
        cyls[i-1]->AddChild(axis[i]);
        axis[i]->Translate(0.8f* scaleFactor,Axis::X);
        // axis[i]->Translate(0.8f* scaleFactor,Axis::Z);
    }
    // cyls[0]->Translate({0,0,0.8f*scaleFactor});
    cyls[0]->Translate({0.8f*scaleFactor,0,0});

    auto morphFunc = [](Model* model, cg3d::Visitor* visitor) {
      return model->meshIndex;//(model->GetMeshList())[0]->data.size()-1;
    };
    autoCube = AutoMorphingModel::Create(*cube, morphFunc);

  
    sphere1->showWireframe = true;
    autoCube->Translate({-6,0,0});
    debug_print(autoCube->GetTranslation());
    autoCube->Scale(1.5f);
//    sphere1->Translate({-2,0,0});

    autoCube->showWireframe = true;
    camera->Translate(22, Axis::Z);
    root->AddChild(sphere1);
//    root->AddChild(cyl);
    root->AddChild(autoCube);
    // points = Eigen::MatrixXd::Ones(1,3);
    // edges = Eigen::MatrixXd::Ones(1,3);
    // colors = Eigen::MatrixXd::Ones(1,3);
    
    // cyl->AddOverlay({points,edges,colors},true);
    cube->mode =1   ; 
    auto mesh = cube->GetMeshList();

    //autoCube->AddOverlay(points,edges,colors);
    // mesh[0]->data.push_back({V,F,V,E});
    int num_collapsed;

  // Function to reset original mesh and data structures
    V = mesh[0]->data[0].vertices;
    F = mesh[0]->data[0].faces;
   // igl::read_triangle_mesh("data/cube.off",V,F);
    igl::edge_flaps(F,E,EMAP,EF,EI);
    std::cout<< "vertices: \n" << V <<std::endl;
    std::cout<< "faces: \n" << F <<std::endl;
    
    std::cout<< "edges: \n" << E.transpose() <<std::endl;
    std::cout<< "edges to faces: \n" << EF.transpose() <<std::endl;
    std::cout<< "faces to edges: \n "<< EMAP.transpose()<<std::endl;
    std::cout<< "edges indices: \n" << EI.transpose() <<std::endl;

}


void BasicScene::ikCyclicCoordinateDecentMethod(std::shared_ptr<AutoMorphingModel> target){
	//TODO - change var names
    //TODO - make sure this works
    //TODO - validate magics - angles and such
    //TODO - remove wrapping if
    //TODO - delete init rotation if it does not help
    //TODO - Remove debugs
    //TODO - implement ALL callback and constraints in the document (ie arm doesnt brake when moving or rotating)

    if (shouldAnimateCCD) {
		Eigen::Vector3f targetDes = target->GetAggregatedTransform().block<3,1>(0,3); //get cube source
        // debug_print("target is at: ");
        // debug_print(targetDes);
		Eigen::Vector3f first_link_pos = ikCylPosition(0, 0); //get first link base
		if ((targetDes - first_link_pos).norm() > CYL_LENGTH * 3) { //if distance of first link base and cube is greater than all links , fail
			std::cout << "cannot reach" << std::endl;
			shouldAnimateCCD = false;
			return;
		}
        int currIndex = lastLinkIndex;
		while (currIndex != -1) {
            std::shared_ptr<Model> currLink = cyls[currIndex];
			Eigen::Vector3f r = ikCylPosition(currIndex, 0);
			Eigen::Vector3f e = ikCylPosition(lastLinkIndex, CYL_LENGTH);
			Eigen::Vector3f rd = targetDes - r;
			Eigen::Vector3f re = e - r;
			Eigen::Vector3f normal = re.normalized().cross(rd.normalized());//calculates plane normal
			float distance = (targetDes - e).norm();
			if (distance < 0.05) {
				std::cout << "done, distance: " << distance << std::endl;
				//initRotation(); 
				shouldAnimateCCD = false;
				return;
			}
			float dot = rd.normalized().dot(re.normalized());
			// check dot prioduct is valid range - joints
			if (dot > 1)
				dot = 1;
			if (dot < -1)
				dot = -1;
			float angle = acosf(dot) / 50;
			Eigen::Vector3f rotationVec = (currLink->GetAggregatedTransform()).block<3, 3>(0, 0).inverse() * normal; //ratate towards link position
			if (currIndex!=0) { //
				currLink->Rotate(angle, rotationVec);
				e = ikCylPosition(lastLinkIndex, CYL_LENGTH); //get new position after rotation
				re = e - r;
				Eigen::Vector3f rParent = ikCylPosition(currIndex-1, 0);
				rd = rParent - r;
				//find angle between parent and link
				double constraint = 0.5236;
				double parentDot = rd.normalized().dot(re.normalized());//get dot 
				if (parentDot > 1)
					parentDot = 1;
				if (parentDot < -1)
					parentDot = 1;
				double parentAngle = acos(parentDot);
				currLink->Rotate(-angle, rotationVec);//rotate back 
				if (parentAngle < constraint) {//fix angle
					angle = angle-(constraint - parentAngle);
				}

			}
			currLink->Rotate(angle, rotationVec);
			currIndex -=1;
		}
	}

}


Eigen::Vector3f BasicScene::ikCylPosition(int target, double length){
    // this returns the cylinder position for ik - depending on length value - returns tip or base of it
    
    Eigen::Matrix4f linkTransform = cyls[target]->GetAggregatedTransform();
	Eigen::Vector3f linkCenter = Eigen::Vector3f(linkTransform.col(3).x(), linkTransform.col(3).y(), linkTransform.col(3).z());
    Eigen::Vector3f r;
	if(length>0) //get tip
        r = linkCenter + cyls[target]->GetRotation() *Eigen::Vector3f(CYL_LENGTH/2,0,0);
    else //get base
        r = linkCenter - cyls[target]->GetRotation() *Eigen::Vector3f(CYL_LENGTH/2,0 ,0);
	return r;
} 


void BasicScene::initRotation(){
	// not sure this is needed yet
	int currLink = lastLinkIndex;
	while (currLink != cyls.size()) {
        Eigen::Matrix3f Z = cyls[currLink]->GetRotation();
		Eigen::Matrix3f R = cyls[currLink]->GetRotation();
		Eigen::Vector3f ea = R.eulerAngles(2, 0, 2);//get the rotation angles
		float angleZ = ea[2];
		cyls[currLink]->Rotate(-angleZ, Axis::Z);
		currLink +=1;
		if (currLink != cyls.size())
			cyls[currLink]->RotateInSystem(Z, angleZ, Axis::Z);
	}
}

void BasicScene::Update(const Program& program, const Eigen::Matrix4f& proj, const Eigen::Matrix4f& view, const Eigen::Matrix4f& model)
{
    Scene::Update(program, proj, view, model);
    program.SetUniform4f("lightColor", 0.8f, 0.3f, 0.0f, 0.5f);
    program.SetUniform4f("Kai", 1.0f, 0.3f, 0.6f, 1.0f);
    program.SetUniform4f("Kdi", 0.5f, 0.5f, 0.0f, 1.0f);
    program.SetUniform1f("specular_exponent", 5.0f);
    program.SetUniform4f("light_position", 0.0, 15.0f, 0.0, 1.0f);
//    cyl->Rotate(0.001f, Axis::Y);
    cube->Rotate(0.1f, Axis::XYZ);
    ikCyclicCoordinateDecentMethod(autoCube);
}

void BasicScene::MouseCallback(Viewport* viewport, int x, int y, int button, int action, int mods, int buttonState[])
{
    // note: there's a (small) chance the button state here precedes the mouse press/release event

    if (action == GLFW_PRESS) { // default mouse button press behavior
        PickVisitor visitor;
        visitor.Init();
        renderer->RenderViewportAtPos(x, y, &visitor); // pick using fixed colors hack
        auto modelAndDepth = visitor.PickAtPos(x, renderer->GetWindowHeight() - y);
        renderer->RenderViewportAtPos(x, y); // draw again to avoid flickering
        pickedModel = modelAndDepth.first ? std::dynamic_pointer_cast<Model>(modelAndDepth.first->shared_from_this()) : nullptr;
        pickedModelDepth = modelAndDepth.second;
        camera->GetRotation().transpose();
        xAtPress = x;
        yAtPress = y;

        // if (pickedModel)
        //     debug("found ", pickedModel->isPickable ? "pickable" : "non-pickable", " model at pos ", x, ", ", y, ": ",
        //           pickedModel->name, ", depth: ", pickedModelDepth);
        // else
        //     debug("found nothing at pos ", x, ", ", y);

        if (pickedModel && !pickedModel->isPickable)
            pickedModel = nullptr; // for non-pickable models we need only pickedModelDepth for mouse movement calculations later

        if (pickedModel)
            pickedToutAtPress = pickedModel->GetTout();
        else
            cameraToutAtPress = camera->GetTout();
    }
}

void BasicScene::ScrollCallback(Viewport* viewport, int x, int y, int xoffset, int yoffset, bool dragging, int buttonState[])
{
    // note: there's a (small) chance the button state here precedes the mouse press/release event
    auto system = camera->GetRotation().transpose();
    if (pickedModel) {
        //change
        std::shared_ptr<cg3d::Model> currModel = pickedModel;
        if (std::find(cyls.begin(), cyls.end(), pickedModel) != cyls.end()) // if picked object is a cylinder pick base - avoid arm breaking
            currModel = cyls[0];
        currModel->TranslateInSystem(system, {0, 0, -float(yoffset)});
        pickedToutAtPress = currModel->GetTout();
    } else {
        camera->TranslateInSystem(system, {0, 0, -float(yoffset)});
        cameraToutAtPress = camera->GetTout();
    }
}

void BasicScene::CursorPosCallback(Viewport* viewport, int x, int y, bool dragging, int* buttonState)
{
    if (dragging) {
        auto system = camera->GetRotation().transpose() * GetRotation();
        auto moveCoeff = camera->CalcMoveCoeff(pickedModelDepth, viewport->width);
        auto angleCoeff = camera->CalcAngleCoeff(viewport->width);
        if (pickedModel) {
            if (buttonState[GLFW_MOUSE_BUTTON_RIGHT] != GLFW_RELEASE){
                std::shared_ptr<cg3d::Model> currModel = pickedModel;
                if(std::find(cyls.begin(), cyls.end(), pickedModel) != cyls.end()) // if picked object is a cylinder pick base - avoid arm breaking
                    currModel = cyls[0];
                currModel->TranslateInSystem(system, {-float(xAtPress - x) / moveCoeff, float(yAtPress - y) / moveCoeff, 0});
            }
            if (buttonState[GLFW_MOUSE_BUTTON_MIDDLE] != GLFW_RELEASE)
                pickedModel->RotateInSystem(system, float(xAtPress - x) / angleCoeff, Axis::Z);
            if (buttonState[GLFW_MOUSE_BUTTON_LEFT] != GLFW_RELEASE) {
                pickedModel->RotateInSystem(system, float(xAtPress - x) / angleCoeff, Axis::Y);
                pickedModel->RotateInSystem(system, float(yAtPress - y) / angleCoeff, Axis::X);
            }
        } else {
            if (buttonState[GLFW_MOUSE_BUTTON_RIGHT] != GLFW_RELEASE)
                root->TranslateInSystem(system, {-float(xAtPress - x) / moveCoeff/10.0f, float( yAtPress - y) / moveCoeff/10.0f, 0});
            if (buttonState[GLFW_MOUSE_BUTTON_MIDDLE] != GLFW_RELEASE)
                root->RotateInSystem(system, float(x - xAtPress) / 180.0f, Axis::Z);
            if (buttonState[GLFW_MOUSE_BUTTON_LEFT] != GLFW_RELEASE) {
                root->RotateInSystem(system, float(x - xAtPress) / angleCoeff, Axis::Y);
                root->RotateInSystem(system, float(y - yAtPress) / angleCoeff, Axis::X);
            }
        }
        xAtPress =  x;
        yAtPress =  y;
    }
}

void BasicScene::KeyCallback(Viewport* viewport, int x, int y, int key, int scancode, int action, int mods)
{
    auto system = camera->GetRotation().transpose();

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) // NOLINT(hicpp-multiway-paths-covered)
        {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        case GLFW_KEY_UP:
            if (std::find(cyls.begin(), cyls.end(), pickedModel) != cyls.end()) {
                cyls[pickedIndex]->RotateInSystem(system, 0.1f, Axis::X);
            }
            else
                root->RotateInSystem(system, 0.1f, Axis::X);
            break;
        case GLFW_KEY_DOWN:
            if (std::find(cyls.begin(), cyls.end(), pickedModel) != cyls.end()) {
                cyls[pickedIndex]->RotateInSystem(system, -0.1f, Axis::X);
            }
            else
                root->RotateInSystem(system, -0.1f, Axis::X);
            break;
        case GLFW_KEY_LEFT:
            if (std::find(cyls.begin(), cyls.end(), pickedModel) != cyls.end()) {
                cyls[pickedIndex]->RotateInSystem(system, 0.1f, Axis::Y);
            }
            else
                root->RotateInSystem(system, 0.1f, Axis::Y);
            break;
        case GLFW_KEY_RIGHT:
            if (std::find(cyls.begin(), cyls.end(), pickedModel) != cyls.end()) {
                cyls[pickedIndex]->RotateInSystem(system, -0.1f, Axis::Y);
            }
            else
                root->RotateInSystem(system, -0.1f, Axis::Y);
            break;
        case GLFW_KEY_W:
            camera->TranslateInSystem(system, { 0, 0.1f, 0 });
            break;
        case GLFW_KEY_S:
            camera->TranslateInSystem(system, { 0, -0.1f, 0 });
            break;
        case GLFW_KEY_A:
            camera->TranslateInSystem(system, { -0.1f, 0, 0 });
            break;
        case GLFW_KEY_D:
            camera->TranslateInSystem(system, { 0.1f, 0, 0 });
            break;
        case GLFW_KEY_B:
            camera->TranslateInSystem(system, { 0, 0, 0.1f });
            break;
        case GLFW_KEY_F:
            camera->TranslateInSystem(system, { 0, 0, -0.1f });
            break;
        case GLFW_KEY_Z: {
            Eigen::Vector3f targetDes = autoCube->GetAggregatedTransform().block<3, 1>(0, 3);
            std::cout << "target = (x: " << targetDes.x() << " y: " << targetDes.y() << " z: " << targetDes.z() << ")" << std::endl; }
                       break;
        case GLFW_KEY_P:
            if (std::find(cyls.begin(), cyls.end(), pickedModel) != cyls.end())
            {
                auto rotation = pickedModel->GetRotation();
                Eigen::Vector3f euler_angles = rotation.eulerAngles(2, 0, 2);
                std::cout << "Phi: " << euler_angles[0] * 180.0f / M_PI << " degrees" << std::endl;
                std::cout << "Theta: " << euler_angles[1] * 180.0f / M_PI << " degrees" << std::endl;
                std::cout << "Psi: " << euler_angles[2] * 180.0f / M_PI << " degrees" << std::endl;
            }
            break;
        case GLFW_KEY_T: {
            Eigen::Vector3f tip = ikCylPosition(lastLinkIndex, CYL_LENGTH);
            std::cout << "target = (x: " << tip.x() << " y: " << tip.y() << " z: " << tip.z() << ")" << std::endl; }
                       break;
        case GLFW_KEY_N: {
            if (pickedIndex < cyls.size() - 1)
                pickedIndex++;
            else
                pickedIndex = 0; }
                break;
            case GLFW_KEY_1:
                if( pickedIndex > 0)
                  pickedIndex--;
                break;
            case GLFW_KEY_2:
                if(pickedIndex < cyls.size()-1)
                    pickedIndex++;
                break;
            case GLFW_KEY_3:
                if( tipIndex >= 0)
                {
                  if(tipIndex == cyls.size())
                    tipIndex--;
                  sphere1->Translate(GetSpherePos());
                  tipIndex--;
                }
                break;
            case GLFW_KEY_4:
                if(tipIndex < cyls.size())
                {
                    if(tipIndex < 0)
                      tipIndex++;
                    sphere1->Translate(GetSpherePos());
                    tipIndex++;
                }
                break;
            case GLFW_KEY_SPACE:
                shouldAnimateCCD = ! shouldAnimateCCD;

                //todo

        }
    }
}

Eigen::Vector3f BasicScene::GetSpherePos()
{
      Eigen::Vector3f l = Eigen::Vector3f(1.6f,0,0);
      Eigen::Vector3f res;
      res = cyls[tipIndex]->GetRotation()*l;   
      return res;  
}



