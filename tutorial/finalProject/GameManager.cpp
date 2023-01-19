#include "GameManager.h"


//TODO implement json configuration loading


void GameManager::InitValues()
{
    SetScore(0);
    currWave=0;
}

void GameManager::GameStart()
{
    // init values that are not highscore
    InitValues();    
    // signal wave manager
}

void GameManager::GameEnd()
{
    // launch menu
    // update high score?
    // see score and high score
    // pause everything
}

void GameManager::NextWave()
{
    currWave++;
    // signal wave manager?
}

void GameManager::Restart()
{
    // end game
    GameEnd();
    // init values
    InitValues();
    // reset position to pre-wave 1
    snake.ResetSnake();
    // launch menu?
}

void GameManager::IncreaseScore(float amount)
{
    score += amount;
    if (score >GetHighScore())
        SetHighScore(score);
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
}

void GameManager::SetScore(float amount){
    score = amount>=0 ? amount : 0
}
