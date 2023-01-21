#pragma once

#include <utility>
#include <vector>
#include <string>
#include <cstdint>
#include "Movable.h"
#include "GameObject.h"

class Snake;

namespace Game{

    class GameManager {


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
            std::vector<std::shared_ptr<Game::GameObject>> gameObjects;
            std::shared_ptr<Snake> snake;

        private:

            float score;
            int currWave;
            float highScore;
            
            void InitValues();
            void SetScore(float amount);
            void SetHighScore(float amount);

    };

}