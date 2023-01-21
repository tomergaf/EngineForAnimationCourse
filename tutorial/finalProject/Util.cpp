
#include "Util.h"
#include "Utility.h"

#define DEBUG true



void Util::DebugPrint(std::string str){
    if (DEBUG)
        cg3d::debug_print(str);
}
