#include <iostream>
#include <thread>
#include <mutex>
#include <algorithm>
#include <vector>
#include <string>
#include <termios.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

typedef std::vector<std::vector<int> > vvi;
typedef std::vector<int> vi;

using namespace std;

mutex readUserInput;
mutex print;
mutex map;
mutex mborda;
char command;

int mapWidth = 20;
int mapLength = 20;

vvi borda;
vi borda_existente;
vi column_kills;

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
int gamespeed = 30000;
int enemyspeed = 1000000;
int playerspeed = 10000;


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

    vi point(2);
    borda.resize(n_enemies_by_line);
    borda_existente.resize(n_enemies_by_line);
    column_kills.resize(n_enemies_by_line);
    for(int i=0; i<borda.size(); i++){
        point[0] = i+1;
        point[1] = n_lines;
        borda[i] = point;
        borda_existente[i] = i;
        column_kills[i] = n_lines;
    }

}

void move_borda(string sentido){

    mborda.lock();
    for(int i=0; i<borda_existente.size(); i++){
        if(sentido == "right"){
            borda[borda_existente[i]][0]++;
        }else if( sentido == "left"){
            borda[borda_existente[i]][0]--;
        }else{ // down
            borda[borda_existente[i]][1]++;
        }
    }
    mborda.unlock();

}

void enemies_move(string &direction, int &leftmost, int &rightmost, bool &flag_down){

    char current_enemy, next_enemy;
    bool first;
    char last_enemy;

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
        move_borda("down");
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
            move_borda(direction);

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
            move_borda(direction);
        }
    }
    map.unlock();
}

void enemies_shot(){
    // insert random shot that fires randomly from the first row.
    mborda.lock();
    map.lock();
    for(int i=0; i<2; i++){
        random_shuffle(borda_existente.begin(), borda_existente.end());
        vi shooter = borda[borda_existente[0]];
        int x = shooter[1];
        int y = shooter[0];
        mapManager.Map[y+1][x] = '!';
    }
    mborda.unlock();
    map.unlock();
}

void enemy_killed(int x, int y){
    int xborda, yborda;
    mborda.lock();
    for(int i=0; i<borda_existente.size(); i++){
        xborda = borda[borda_existente[i]][1];
        yborda = borda[borda_existente[i]][0];

        if( (x == xborda) && (y == yborda) ){
            column_kills[borda_existente[i]]--;
            if(column_kills[borda_existente[i]] <= 0){
                borda[borda_existente[i]][0] = -1;
                borda[borda_existente[i]][1] = -1;
                borda_existente.erase(borda_existente.begin() + i);
            }else{
                borda[borda_existente[i]][0]--;
            }
            break;
        }
    }
    mborda.unlock();
}

// Thread da atualizacao dos inimigos(em construcao)
void enemy_update(){
     // 11 inimigos por linha
    // 5 linhas

    int n_enemies_by_line = 10;
    int n_lines = 5;
    string direction = "right";
    int leftmost = 1;
    int rightmost = n_enemies_by_line;

    bool flag_down = false;

    create_enemies(n_lines, n_enemies_by_line);

    while(!endgame){
        enemies_move(direction, leftmost, rightmost, flag_down);
        enemies_shot();
        usleep(enemyspeed);
    }

}

// Para saber qual os inimigos mais a esquerda ou a direita tem que contar fazer um vetor 
//com a contagem de
// quais inimigos daquela fileira morreu.


// Thread de refresh da tela de jogo.
void _refresh(){ // tem que colocar o timer de 50ms pra atualizacao obrigatoria.

    while(!endgame){
        system("clear");
        mapManager.printMap();
        usleep(gamespeed);
    }
}

void playerControl(){

    bool update_once;
    bool update_shot;
    while(!endgame){
        map.lock();
        update_once = true;
        update_shot = true;
        for(int y = 19; y > 0; y--){
            for(int x = 19; x > 0; x--){

                switch(mapManager.Map[y][x]){

                    case 'A':
                        //sync here
                        //Moves Left
                        if(command == 's' || command == 'S') move(x-1, true, x, y);
                        

                        //Moves Right
                        if(command == 'd' || command == 'D') move(x+1, false, x, y);
                        

                        if(command == 'l' || command == 'L'){
                            mapManager.Map[y-1][x] = '^';
                            command = ' ';
                        }

                    break;

                    case '^':
                        if(update_once){
                            update_once = false;
                            mapManager.Map[y][x] = ' ';

                            if(mapManager.Map[y-1][x] == 'W' || mapManager.Map[y-1][x] == 'Y' ||
                               mapManager.Map[y-1][x] == 'U' || mapManager.Map[y-1][x] == 'V'){
                                // Atualizar bordas
                                enemy_killed(x, y-1);
                                mapManager.Map[y-1][x] = ' ';
                            }else if(mapManager.Map[y-1][x] == ' '){
                                mapManager.Map[y-1][x] = '^';
                            }else{
                                y=y; // Nada acontece
                            }
                        }else{
                            update_once = true;
                        }

                    break;

                    case '!':
                        if(update_shot){
                            update_shot = false;
                            mapManager.Map[y][x] = ' ';
                            if(mapManager.Map[y+1][x] == 'A'){
                                // fim do jogo
                                int q = 0;
                            }else if(mapManager.Map[y+1][x] == ' '){
                                mapManager.Map[y+1][x] = '!';
                            }else{
                                y = y;
                            }
                        }else{
                            update_shot = true;
                        }
                }
            }
        }
        map.unlock();
        usleep(playerspeed);
    }
}

int main(){

    // Dispara threads para iniciar o programa.
    thread input(readInput);
    thread refresh(_refresh);
    thread player(playerControl);
    thread enemy(enemy_update);
    //thread logger();

    while(!endgame){
        // While loop que segura o jogo ate que o mesmo
        // seja abortado.
    }

    // Join das thread ativas.
    player.detach();
    refresh.detach();
    enemy.detach();
    input.detach();

    return 0;
}
