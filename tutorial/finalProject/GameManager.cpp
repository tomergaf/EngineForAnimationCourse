#include "GameManager.h"
#include "Util.h"
#include "Snake.h"
#include "SpawnManager.h"



namespace Game{
//TODO implement json configuration loading

    GameManager::GameManager(SpawnManager* spawnManager) 
    {
        this->spawnManager = spawnManager;
        shouldSpawnNextWave = false;
        this->scene = spawnManager->scene;
        SetHighScore(0);
        InitValues();
    }

    void GameManager::InitValues()
    {
        SetScore(0);
        currWave=0;
    }


    void GameManager::GameStart()
    {
        // init values that are not highscore
        Util::DebugPrint("Game Starting...");
        InitValues(); 
        scene->SetActive();  
        scene->snake->InitGameValues();  
        // signal wave manager
        NextWave();
        //log this
    }

    void GameManager::GameEnd()
    {
        // launch menu
        // update high score?
        Util::DebugPrint("Score is: " + std::to_string(score));
        // see score and high score
        scene->SetActive(false);
        // pause everything
    }

    void GameManager::NextWave()
    {
        currWave++;
        //TEMP change to last wave number
        if(currWave == 6){
            Util::DebugPrint("Congratulations! you have reached the end!");
            GameEnd();
        }
        else{
            Util::DebugPrint("Wave Started: " + std::to_string(GetCurrWave()));
            spawnManager->SpawnWave(GetCurrWave());
        }
        shouldSpawnNextWave=false;
        // signal wave manager?
    }

    void GameManager::Restart()
    {
        // end game
        GameEnd();
        // init values
        InitValues();
        
        // reset position to pre-wave 1
        // snake->ResetSnake();
        // launch menu?
    }

    void GameManager::IncreaseScore(float amount)
    {
        score += amount;
        if (score >GetHighScore())
            SetHighScore(score);
        Util::DebugPrint("score is: " + std::to_string(score));
    }

    void GameManager::DecreaseScore(float amount)
    {
        score = (score-amount)>=0 ? score-amount : 0;
    }

    float GameManager::GetHighScore()
    {
        return highScore;
    }

    float GameManager::GetScore()
    {
        return score;
    }

    int GameManager::GetCurrWave()
    {
        return currWave;
    }

    void GameManager::SetHighScore(float amount)
    {
        highScore = amount;
        Util::DebugPrint("High score is: " + std::to_string(score));
    }

    void GameManager::SetScore(float amount){
        score = amount>=0 ? amount : 0;
    }
}