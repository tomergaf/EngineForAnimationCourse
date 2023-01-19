#pragma once

#include <utility>
#include <vector>
#include <string>
#include <cstdint>
#include "Snake.h"

class GameManager{


    public:
        
        GameManager();

        void GameStart();
        void GameEnd();
        void NextWave();
        void Restart();
        void IncreaseScore(float amount);
        void DecreaseScore(float amount);
        float GetHighScore();
        float GetScore();
        int GetCurrWave();

    private:

        float score;
        int currWave;
        float highScore;
        Snake snake;
        
        void InitValues();
        void SetScore(float amount);
        void SetHighScore(float amount);

};