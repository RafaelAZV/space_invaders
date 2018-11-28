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

int mapWidth = 20;
int mapLength = 20;

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
int gamespeed = 1000000;




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

void create_enemies(int n_lines, int n_enemies_by_line){

    map.lock();
    for(int y=1; y<(n_lines+1); y++){
        for(int x=1; x<(n_enemies_by_line+1); x++){
                switch(y){
                    case 1:
                        mapManager.Map[y][x] = 'Y';
                        break;
                    case 2:
                        mapManager.Map[y][x] = 'U';
                        break;
                    case 3:
                        mapManager.Map[y][x] = 'V';
                        break;
                    default:
                        mapManager.Map[y][x] = 'W';
                }
            }
    }
    map.unlock();

}

void enemies_move(string &direction, int &leftmost, int &rightmost, bool &flag_down){

    char current_enemy, next_enemy;
    bool first;
    char last_enemy;

    cout << flag_down << endl;

    map.lock();
    // Movimenta para baixo
    if( ((rightmost >= (mapWidth-3)) || (leftmost <= 2) ) && flag_down == true){
        for(int x=1; x<mapWidth-2; x++){
            first = true;
            for(int y=1; y<mapLength-2; y++){
                if(mapManager.Map[y][x] != 'A' && mapManager.Map[y][x] != '^'){
                    // Salva caractere 
                    current_enemy = mapManager.Map[y][x];
                    // primeiro da linha
                    if(first){ 
                        mapManager.Map[y][x] = ' ';
                        last_enemy = current_enemy;
                        first = false;
                    //outros
                    }else{
                        mapManager.Map[y][x] = last_enemy;
                        last_enemy = current_enemy;
                    }
                }
            }
        }    
        // troca de sentido
        flag_down = false;
        if(direction == "right"){
            direction = "left";
        }else{
            direction = "right";
        }
    }else{
        // movimenta para a direita
        if(direction == "right"){ 
            //flag_down = true;
            for(int y=1; y<mapWidth-2; y++){
                first = true;
                for(int x=1; x<mapLength-2; x++){
                    if(mapManager.Map[y][x] != 'A' && mapManager.Map[y][x] != '^'){
                        // Salva caractere 
                        current_enemy = mapManager.Map[y][x];
                        // primeiro da linha
                        if(first){ 
                            mapManager.Map[y][x] = ' ';
                            last_enemy = current_enemy;
                            first = false;
                        //outros
                        }else{ 
                            mapManager.Map[y][x] = last_enemy;
                            last_enemy = current_enemy;
                        }
                    }
                }
            }
            rightmost++;
            leftmost++;
            if(rightmost == mapWidth-3){
                flag_down = true;
            }

        }else{
            //flag_down = true;
            // movimenta para a esquerda
            for(int y=(mapLength-3); y>0; y--){
                first = true;
                for(int x=(mapWidth-3); x>0; x--){
                    if(mapManager.Map[y][x] != 'A' && mapManager.Map[y][x] != '^'){
                        // Salva caractere 
                        current_enemy = mapManager.Map[y][x];
                        // primeiro da linha
                        if(first){ 
                            mapManager.Map[y][x] = ' ';
                            last_enemy = current_enemy;
                            first = false;
                        //outros
                        }else{ 
                            mapManager.Map[y][x] = last_enemy;
                            last_enemy = current_enemy;
                        }
                    }
                }
            }
            rightmost--;
            leftmost--;
            if(leftmost == 1){
                flag_down = true;
            }
        }
    }
    map.unlock();
}

// Thread da atualizacao dos inimigos(em construcao)
void enemy_update(){
     // 11 inimigos por linha
    // 5 linhas
    int n_enemies_by_line = 10;
    int n_lines = 5;
    string direction = "right";
    int leftmost = 3;
    int rightmost = n_enemies_by_line;

    create_enemies(n_lines, n_enemies_by_line);

    /*while(!endgame){
        enemies_move(direction, leftmost, rightmost);
    }*/

}

void enemies_shot(){
    int i = 0;
    // insert random shot that fires from the first row.
}

// Para saber qual os inimigos mais a esquerda ou a direita tem que contar fazer um vetor com a contagem de
// quais inimigos daquela fileira morreu.


// Thread de refresh da tela de jogo.
void _refresh(){ // tem que colocar o timer de 50ms pra atualizacao obrigatoria.

    int n_enemies_by_line = 10;
    int n_lines = 5;
    string direction = "right";
    int leftmost = 1;
    int rightmost = n_enemies_by_line;

    int i = 0;
    bool flag_down = false;
    //while(!endgame){
    while(i < 30){
        system("clear");
        cout << "MOVE" << endl;
        enemies_move(direction, leftmost, rightmost, flag_down);
        mapManager.printMap();
        usleep(gamespeed);
        i++;
    }
    cout << "saiu do loop" << endl;
}


void playerControl(){

    while(!endgame){
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
}

int main(){

    // Dispara threads para iniciar o programa.
    thread input(readInput);
    thread player(playerControl);
    thread enemy(enemy_update);
    thread refresh(_refresh);
    //thread logger();

    while(!endgame){
        // While loop que segura o jogo ate que o mesmo
        // seja abortado.
    }

    // Join das thread ativas.
    player.join();
    refresh.join();
    input.join();
    enemy.join();

    return 0;
}
