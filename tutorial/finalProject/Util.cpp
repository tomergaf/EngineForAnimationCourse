
#include "Util.h"
#include "Utility.h"
#include <random>
#define DEBUG true



void Util::DebugPrint(std::string str){
    if (DEBUG)
        cg3d::debug_print(str);
}


float Util::GenerateRandomInRange(float min, float max)
{
    std::random_device randDevice;
	std::mt19937       randGenerator(randDevice());
	std::uniform_real_distribution<> distribute(min, max);
    return distribute(randGenerator);
}

