#include <windows.h>
#include <cstdio>
#include <iostream>

#include "SoundManager.h"
#include <string>
#include <mmsystem.h>
#pragma comment( lib, "Winmm.lib" )



void SoundManager::PlayHitSound()
{
    std::string strpath = "Sounds\\Hit.wav";
    PlayGenericSound(strpath);
}
void SoundManager::PlayPickupSound()
{
    std::string strpath = "Sounds\\Pickup.wav";
    PlayGenericSound(strpath);
}
void SoundManager::PlayGameEndSound()
{
    std::string strpath = "Sounds\\GameEnd.wav";
    PlayGenericSound(strpath);
}

void SoundManager::PlayGenericSound(std::string path)
{
    PlaySound(path.c_str(), NULL, SND_FILENAME | SND_ASYNC);
}
