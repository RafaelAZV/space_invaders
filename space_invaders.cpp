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
#include <sstream>          // parsing
#include <sys/socket.h> // for UDP
#include <netinet/in.h> // for UDP          // for File output
#include <sys/time.h>       // Time measurement
#include <strings.h>
#include <string.h>
#include <sys/wait.h>
#include <ctime>
#include <ratio>
#include <chrono>
#include <fstream>

#define UDP_FILE_LOG_FLAG 1

using namespace std;
using namespace std::chrono;
typedef std::vector<std::vector<int> > vvi;
typedef std::vector<int> vi;

int current_pontuation = 0;

mutex points;
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



/*
Class containing the map used in the user interface
*/

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


    /*for(int i=0; i<borda.size(); i++){
        cout << borda[i][0] << " " << borda[i][1] << endl;
    }
    cout << borda_existente.size() << endl;
    for(int i=0; i<borda_existente.size(); i++){
        cout << borda_existente[i] << " ";
    }
    cout << endl;*/

}

Map mapManager;


/*
Class responsable for receiving the user input from keyboard and allow the game to continue
*/
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


/*
GLOBAL VARIABLES USED IN GAME
*/
bool endgame = false;
bool won = false;
int gamespeed = 30000;
int enemyspeed = 1000000;
int playerspeed = 10000;


/*
    END WORKTERMIOS CLASS
*/



void readInput(){

    WorkTermios inputManager;

    while(!endgame){
        readUserInput.lock();
        command = inputManager.getch();
        readUserInput.unlock();
    }
}



/*
AUXILIAR FUNCTION TO MOVE THE PLAYER IN MAP
*/
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

/*
Function to spawn enemies in map
*/
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
    mborda.lock();
    borda.resize(n_enemies_by_line);
    borda_existente.resize(n_enemies_by_line);
    column_kills.resize(n_enemies_by_line);
    for(int i=0; i<borda.size(); i++){
        point[0] = n_lines;
        point[1] = i+1;
        borda[i] = point;
        borda_existente[i] = i;
        column_kills[i] = n_lines;
    }
    mborda.unlock();

}


void move_borda(string sentido){

    mborda.lock();
    for(int i=0; i<borda_existente.size(); i++){
        if(sentido == "right"){
            borda[borda_existente[i]][1]++;
        }else if( sentido == "left"){
            borda[borda_existente[i]][1]--;
        }else{ // down
            borda[borda_existente[i]][0]++;
        }
    }
    mborda.unlock();

}

void update_leftmost_rightmost(int &leftmost, int &rightmost){
    mborda.lock();

    sort(borda_existente.begin(), borda_existente.end());

    int len = borda_existente.size()-1;

    leftmost = borda[borda_existente[0]][1];
    rightmost = borda[borda_existente[len]][1];

    mborda.unlock();
}

/*
Function to move the enemies in the direction pattern
*/
void enemies_move(string &direction, int &leftmost, int &rightmost, bool &flag_down){

    char current_enemy, next_enemy;
    bool first;
    char last_enemy;

    update_leftmost_rightmost(leftmost, rightmost);

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

/*
Manage enemy dynamics when one gets killed
*/
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

    if(borda_existente.size() == 0){
        endgame = true;
        won = true;
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
void _refresh(){ 

    while(!endgame){
        system("clear");
        mapManager.printMap();
        usleep(gamespeed);
    }

}


/*
Method controlled by the thread player
*/
void playerControl(){

    bool update_once;
    bool update_shot;
    int count; // Previne bugs.
    while(!endgame){
        map.lock();
        update_once = true;
        update_shot = true;
        count = 0;
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
                               points.lock();
                               current_pontuation += 1;
                               points.unlock();
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
                                mapManager.Map[y+1][x] == ' ';
                                endgame = true;
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


/*
UDP server that represents the Logger abstraction
*/
class UDP_Thread_Server {

private:

    int sockfd,n;           // UDP
    struct sockaddr_in servaddr,cliaddr;    // UDP
    socklen_t len;          // UDP

public:

    void InternalThreadEntryFunc();
    void init();
};


/*
UDP client that send data to server
*/
class UDP_Thread_Client {

private:

    int sockfd,n;           // UDP
    struct sockaddr_in servaddr,cliaddr;    // UDP
    socklen_t len;          // UDP

public:

    void InternalThreadEntryFunc();
    void LifeCheck();
    void init();
};

/*
Initialize conexion parameters
*/
void UDP_Thread_Client::init(){

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    bzero(&servaddr,sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(14654);
    servaddr.sin_addr.s_addr = INADDR_ANY;

}

/*
Function to send data to server using sendto()
*/
void UDP_Thread_Client::InternalThreadEntryFunc() {

    int buffer[1024];

    for(;;){

        sleep(5);

        buffer[0] = 0;
        buffer[1] = current_pontuation;

        sendto(sockfd, buffer, sizeof(buffer),
            MSG_CONFIRM, (const struct sockaddr *) &servaddr,
                sizeof(servaddr));

        if(endgame) break;
    }

}

void UDP_Thread_Client::LifeCheck() {

    int buffer[1024];

    for(;;){

        if(endgame){

            buffer[0] = 1;
            buffer[1] = 0;

            sendto(sockfd, buffer, sizeof(buffer),
                MSG_CONFIRM, (const struct sockaddr *) &servaddr,
                    sizeof(servaddr));

            break;
        }

        
    }

}

/*
Initialize conexion parameters
*/
void UDP_Thread_Server::init () {

    // udp initialization
    sockfd=socket(AF_INET,SOCK_DGRAM,0);
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
    servaddr.sin_port=htons(14654);
    bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

}

char* GetCurrentTime(){
    system_clock::time_point today = system_clock::now();
    time_t aux_time = system_clock::to_time_t (today);

    return ctime(&aux_time);
}


/*
Writes in log file accorindg to command received
*/
void WriteLog(int buffer[1024]){

    int command = buffer[0];

    std::string init_base_string = "Início do jogo: ";
    std::string pontuation_base_string = "Pontuacao Corrente: ";
    std::string destruction_base_string = "  Instante de Destruicao: ";
    std::string life_base_string = "Vidas Restantes: ";
    std::string end_base_string = "Hora do fim do jogo: ";

    std::ofstream out("logs.txt", std::ios_base::app);

    switch(command){

        case 0:
            {
                std::string number = std::to_string(buffer[1]);
                pontuation_base_string += number;
                out << pontuation_base_string;
                out << std::endl;
            }
        break;

        case 1:
            {
                std::string number = std::to_string(buffer[1]);
                std::string current_time(GetCurrentTime());
                life_base_string += number;
                life_base_string += " ";

                destruction_base_string += current_time;
                life_base_string += destruction_base_string;

                out << life_base_string;
            }
        break;

        case 2:
            {
                std::string current_time(GetCurrentTime());
                end_base_string += current_time;
                out << end_base_string;
            }
        break;

        case 3:
            {
                std::string current_time(GetCurrentTime());
                init_base_string += current_time;
                out << init_base_string;
            }
        break;
    }

    out.close();
}

/*
Server function that waits a message arrive using recvfrom()
*/

void UDP_Thread_Server::InternalThreadEntryFunc() {

    int buffer[1024];
    int n;
    socklen_t len;

    for (;;){
        len = sizeof(cliaddr);
        n = recvfrom(sockfd, buffer, sizeof(buffer),  MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);
        WriteLog(buffer);
        if(endgame) break;
    }
}

void StartCommunicationServer(){
    UDP_Thread_Server abc;
    abc.init();

    int hackbuff[2];
    hackbuff[0] = 3;

    WriteLog(hackbuff);

    abc.InternalThreadEntryFunc();
}

void StartCommunicationClient(){

    UDP_Thread_Client abc;
    abc.init();

    abc.InternalThreadEntryFunc();
    abc.LifeCheck();
}


/*
Main Class to create the children process Logger and manage the parent process to do the other game tasks
*/
class HandleChildProcess {

private:

    int pid;
    int status;

public:
    void init();
};

void HandleChildProcess::init(){

    pid = fork();
    int hackbuff[1];
    hackbuff[0] = 2;

    switch (pid){

        case -1: // error
            perror("fork");
            exit(1);

        case 0: // child process
            StartCommunicationServer();
            exit(0);

        default: // parent process, pid now contains the child pid

            thread comm(StartCommunicationClient);
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
            player.join();
            refresh.join();
            enemy.join();

            if(won){
                cout << "PARABENS!! VOCE PROTEGEU A TERRA DA AMEACA ALIENIGENA!! " << endl;
            }

            usleep(2000);

            comm.join();

            cout << "\n\nFIM DO JOGO! PRESSIONE QUALQUER TECLA PARA SAIR!" << endl;
            WriteLog(hackbuff);
            input.join();

            exit(0);
        break;
    }

}

int main(){

    HandleChildProcess game;
    game.init();
    return 0;
}
