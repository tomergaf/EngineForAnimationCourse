#pragma once


#include <iostream>
#include <windows.h>
#include <mmsystem.h>
#include <string>
#pragma comment( lib, "Winmm.lib" )

class SoundManager{

    public:
        static void PlayHitSound();
        static void PlayPickupSound();
        static void PlayGameEndSound();

        static void PlayGenericSound(std::string path);
    private:
};