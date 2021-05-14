#include<unistd.h>
#include<termios.h>
#include<stdlib.h>
#include<ctype.h>
#include<stdio.h>
#include<errno.h>
#include<string>
#include<sys/ioctl.h>
#include<time.h>
#include<string>
#include<string.h>
#include<chrono>
#include<thread>
#include<iostream>
#include<mutex>
#define REF_LEN 4
#define OUT_MSG "Press q: quit, r: restart"
#define WIN_MSG "You Won, Press q: quit, r: restart"

void updateReflector();
void renderGrid();
void refreshScreen();
char** generateGrid();
void initGame();

struct termios orgTermios;
struct winsize ws;
int row;
int col;
char **bricks;
int refX;
int refY;
int ballVelX;
int ballVelY;
int ballPosX;
int ballPosY;
long long score;
bool notOut;
int totalBricks;

void error(const char *msg){
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(msg);
    exit(1);
}

void disableRawMode(){
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &orgTermios) == -1)
        error("tcsetattr error");
    write(STDOUT_FILENO, "\x1b[?25h", 6);
}

void enableRawMode(){
    if(tcgetattr(STDIN_FILENO, &orgTermios) == -1)
        error("tsgetattr error");
    atexit(disableRawMode);
    struct termios rawTermios = orgTermios;
    rawTermios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    rawTermios.c_oflag &= ~(OPOST);
    rawTermios.c_cflag |= (CS8);
    rawTermios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    rawTermios.c_cc[VMIN] = 0;
    rawTermios.c_cc[VTIME] = 1;
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &rawTermios) == -1)
        error("tcsetattr error");
    write(STDOUT_FILENO, "\x1b[?25l", 6);
}

char readKeyPress(){
    int nread;
    char c;
    while((nread = read(STDIN_FILENO, &c, 1)) != 1){
        if (nread == -1 && errno != EAGAIN)
            error("read error");
    }
    return c;
}

void processKeyPress(){
    char ch = readKeyPress();
    switch(ch) {
        case 'q':
            free(bricks);
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            write(STDOUT_FILENO, "\x1b[?25h", 6);
            exit(0);
            break;
        case 'r':
            if(!notOut){
                free(bricks);
                initGame();
            }
            break;
        case 'a':
            if(notOut){
                refX--;
                refX = (refX<0?0:refX);
                updateReflector();
                break;
            }
        case 'd':
            if(notOut){
                refX++;
                refX = (refX+REF_LEN>=col?col-1-REF_LEN:refX);
                updateReflector();
                break;
            }
        default:
            processKeyPress();
            break;
    }
}

void printOutWinMsg(std::string msg){
    std::string outMsg[] = {"Ouch! ", "Howzat! ", "Mind the line! ", "Focus! ", "B.L.N.T.! ", "Fragile! "};
    int outMsgLen = 6;
    srand(time(0));
    std::string str = (totalBricks>0?outMsg[(int)rand()%outMsgLen]:"") + msg;
    strcpy(&bricks[row-1][13], str.c_str());
}

void updateScore(){
    std::string scoreStr =  "Score: " + std::to_string(score);
    strcpy(bricks[row-1], scoreStr.c_str());
}

void getWindowSize(){
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
        error("error getting window size");
    }
}

void moveBall(){
    while(true){
        bool moveReq = true;
        int tempX = ballPosX + ballVelX;
        int tempY = ballPosY + ballVelY;
        if(tempX==0 || tempY==0 || tempX==col-1 || tempY==row-2){
            if(tempX == 0){
                ballVelX = 1;
                tempX = 0;
            }
            if(tempY == 0){
                ballVelY = 1;
                tempY = 0;
            }
            if(tempY == row-2){
                if(tempX>=refX && tempX<=refX+REF_LEN){
                    ballVelY = -1;
                    moveReq = false;
                }
                else{
                    bricks[ballPosY][ballPosX] = ' ';
                    ballPosX = tempX;
                    ballPosY = tempY;
                    bricks[ballPosY][ballPosX] = 'O';
                    notOut = false;
                    printOutWinMsg(OUT_MSG);
                    refreshScreen();
                    processKeyPress();
                    continue;
                }
            }
            if(tempX == col-1){
                ballVelX = -1;
                tempX = col-1;
            }
        }
        else if(bricks[tempY][tempX] != ' '){
            ballVelX = -1*ballVelX;
            ballVelY = -1*ballVelY;
            bricks[tempY][tempX] = ' ';
            tempX = ballPosX;
            tempY = ballPosY;
            totalBricks--;
        }
        else if(bricks[tempY][tempX-1] != ' '){
            ballVelX = 1;
            bricks[tempY][tempX-1] = ' ';
            totalBricks--;
        }
        else if(bricks[tempY-1][tempX] != ' '){
            ballVelY = 1;
            bricks[tempY-1][tempX] = ' ';
            totalBricks--;
        }
        else if(bricks[tempY][tempX+1] != ' '){
            ballVelX = -1;
            bricks[tempY][tempX+1] = ' ';
            totalBricks--;
        }
        else if(bricks[tempY+1][tempX] != ' ' && bricks[tempY+1][tempX] != '_'){
            ballVelY = -1;
            if(tempY+1 != row-2){
                bricks[tempY+1][tempX] = ' ';
                totalBricks--;
            }
        }
        if(moveReq){
            bricks[ballPosY][ballPosX] = ' ';
            ballPosX = tempX;
            ballPosY = tempY;
            bricks[ballPosY][ballPosX] = 'O';
        }
        if(totalBricks == 0){
            notOut = false;
            printOutWinMsg(WIN_MSG);
            refreshScreen();
            processKeyPress();
            continue;
        }
        score++;
        refreshScreen();
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
}

void updateReflector(){
    for(int i=0;i<col;i++){
        if(i>=refX && i<=refX+REF_LEN){
            bricks[refY][i] = '=';
        }
        else{
            bricks[refY][i] = '_';
        }
    }
}

char** generateGrid(){
    char brickTypes[] = {'*','@','#','$','%','&', ' '};
    int btsize = 6;
    char **bricks = new char*[row];
    srand(time(0));
    for(int i=0;i<row/2;i++){
        bricks[i] = new char[col];
        if(i==row/4)btsize++;
        for(int j=0;j<col;j++){
            if(i>0 && bricks[i-1][j] == ' '){
                bricks[i][j] = ' ';
            }
            else{
                bricks[i][j] = brickTypes[(int)(rand()%btsize)];
                totalBricks += (bricks[i][j]==' '?0:1);
            }
        }
    }
    for(int i=row/2;i<row;i++){
        bricks[i] = new char[col];
        for(int j=0;j<col;j++){
            bricks[i][j] = ' ';
        }
    }
    for(int i=0;i<=col;i++){
        if(i>=refX && i<=refX + REF_LEN){
            bricks[refY][i] = '=';
        }
        else{
            bricks[refY][i] = '_';
        }
    }
    bricks[ballPosY][ballPosX] = 'O';
    return bricks;
}

void renderGrid(){
    char *render = (char *)malloc(1);
    int renderLen = 0;
    for(int i=0;i<row;i++){
        render = (char *)realloc(render, renderLen+col+5);
        memcpy(&render[renderLen], "\x1b[K", 3);
        memcpy(&render[renderLen+3], bricks[i], col);
        if(i<row-1){
            memcpy(&render[renderLen+col+3], "\r\n", 2);
            renderLen += 2;
        }
        renderLen += col+3;
    }
    write(STDOUT_FILENO, render, renderLen);
    free(render);
}

std::mutex mtx;

void refreshScreen(){
    mtx.lock();
    updateScore();
    renderGrid();
    write(STDOUT_FILENO, "\x1b[H", 3);
    mtx.unlock();
}

void initGame(){
    getWindowSize();
    notOut = true;
    score = 0;
    row = ws.ws_row;
    col = ws.ws_col;
    refX = col/2-2;
    refY = row-2;
    ballVelX = -1;
    ballVelY = -1;
    ballPosY = refY-1;
    ballPosX = refX+2;
    totalBricks = 0;
    bricks = generateGrid();
}

int main(){
    enableRawMode();
    initGame();
    std::thread ballMoveThread(moveBall);
    while(true){
        updateScore();
        refreshScreen();
        processKeyPress();
        std::this_thread::yield();
    }
    return 0;
}