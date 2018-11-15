#include <iostream>
#include <thread>
#include <mutex>
#include <string>
#include <termios.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

using namespace std;

mutex readUserInput;
mutex print;
mutex map;
char command;

class Map{

    public:

    char Map[20][20] = {
    "###################",
    "#                 #",
    "#                 #",
    "#                 #",
    "#                 #",
    "#                 #",
    "#                 #",
    "#                 #",
    "#                 #",
    "#                 #",
    "#                 #",
    "#                 #",
    "#                 #",
    "#                 #",
    "#                 #",
    "#                 #",
    "#                 #",
    "#                 #",
    "#       A         #",
    "###################",
    };

    void printMap();
};

void Map::printMap(){

    for(int y = 0; y < 20; y++){
        std::cout << this->Map[y] << std::endl;
    }
}

Map mapManager;

class WorkTermios{

    public:

    struct termios old;
    struct termios _new;

    void initTermios(int echo);
    void resetTermios();
    char getch_(int echo);
    char getch();
    char getche();
};

/* Initialize new terminal i/o settings */
void WorkTermios::initTermios(int echo){


    tcgetattr(0, &old); /* grab old terminal i/o settings */
    _new = old; /* make new settings same as old settings */
    _new.c_lflag &= ~ICANON; /* disable buffered i/o */

    if (echo) {
        _new.c_lflag |= ECHO; /* set echo mode */
    }
    else {
        _new.c_lflag &= ~ECHO; /* set no echo mode */
    }

    tcsetattr(0, TCSANOW, &_new); /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void WorkTermios::resetTermios(void){
  tcsetattr(0, TCSANOW, &old);
}

/* Read 1 character - echo defines echo mode */
char WorkTermios::getch_(int echo){

  char ch;

  initTermios(echo);
  ch = getchar();
  resetTermios();
  return ch;
}

/* Read 1 character without echo */
char WorkTermios::getch(void){
  return getch_(0);
}

/* Read 1 character with echo */
char WorkTermios::getche(void){
  return getch_(1);
}

bool endgame = false;
int gamespeed = 3000;




/*
    TODO: PUT EVERYTHING ABOVE IN CLASSES PACKAGES
*/



void readInput(){

    WorkTermios inputManager;

    while(1){
        readUserInput.lock();
        command = inputManager.getch();
        readUserInput.unlock();
    }
}


void move(int newX, bool left, int x, int y){

    switch(mapManager.Map[y][newX]){

        case ' ':
            mapManager.Map[y][x] = ' ';
            if(left) x--; else x++;
            mapManager.Map[y][newX] = 'A';
        break;
    }

    command = ' ';

}

void playerControl(){

    map.lock();
    for(int y = 0; y < 20; y++){
        for(int x = 0; x < 20; x++){

            switch(mapManager.Map[y][x]){

                case 'A':
                    //sync here
                    //Moves Left
                    if(command == 's' || command == 'S') move(x-1, true, x, y);
                    

                    //Moves Right
                    if(command == 'd' || command == 'D') move(x+1, false, x, y);
                    

                    if(command == 'l' || command == 'L'){
                        y--;
                        mapManager.Map[y][x] = '^';
                        command = ' ';
                    }

                break;

                case '^':

                    mapManager.Map[y][x] = ' ';
                    y--;

                    if(mapManager.Map[y][x] != '#'){
                        mapManager.Map[y][x] = '^';
                    }

                break;
            }
        }
    }
    map.unlock();
}

int main(){

    thread input(readInput);
    input.detach();

    while(!endgame){

        //there should be a thread only to run these 2 lines
        system("clear");
        mapManager.printMap();

        thread player(playerControl);
        player.join();

        //probably another to run this
        usleep(gamespeed);
    }


    return 0;
}
