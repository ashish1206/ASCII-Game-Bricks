#include<unistd.h>
#include<termios.h>
#include<stdlib.h>
#include<ctype.h>
#include<stdio.h>
#include<errno.h>
#include<string>
#include<sys/ioctl.h>
#include<time.h>
#include<string.h>
#include<chrono>
#define REF_LEN 4
using namespace std;

void updateReflector();

struct termios orgTermios;
struct winsize ws;
int row;
int col;
char **bricks;
int refX;
int ballVelX;
int ballVelY;
int ballPosX;
int ballPosY;
auto ball_last_time = chrono::high_resolution_clock::now();

void error(const char *msg){
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(msg);
    exit(1);
}

//Ternimal
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
    rawTermios.c_cc[VTIME] = 5;
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &rawTermios) == -1)
        error("tcsetattr error");
    write(STDOUT_FILENO, "\x1b[?25l", 6);
}

//Key processing
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
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
        case 'a':
            refX--;
            refX = (refX<0?0:refX);
            updateReflector();
            break;
        case 'd':
            refX++;
            refX = (refX+REF_LEN>col?col-REF_LEN:refX);
            updateReflector();
            break;
    }
}

void getWindowSize(){
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
        error("error getting window size");
    }
}

//Grid gen and update
long long millisSinceBallLastMove(){
    auto now_time = chrono::high_resolution_clock::now();
    long long diff_in_millis = chrono::duration_cast<chrono::microseconds>(now_time - ball_last_time).count();
    return diff_in_millis;
}

void moveBall(){
    long long milli = millisSinceBallLastMove();
    if(milli >= 1000){
        int tempX = ballPosX + ballVelX;
        int tempY = ballPosY + ballVelY;
        if(tempX==0 || tempY==0 || tempX==row-1 || tempY==col-1){
            if(tempX == 0){
                ballVelX = 1;
                tempX = 0;
            }
            if(tempY == 0){
                ballVelY = 1;
                tempY = 0;
            }
            if(tempX == row-1){
                ballVelX = -1;
                tempX = row-1;
            }
            if(tempY == col){
                ballVelY = -1;
                tempY = col-1;
            }
        }
        else if(bricks[tempY][tempX-1] != ' '){
            ballVelX = 1;
            bricks[tempY][tempX-1] = ' ';
        }
        else if(bricks[tempY-1][tempX] != ' '){
            ballVelY = 1;
            bricks[tempY-1][tempX] = ' ';
        }
        else if(bricks[tempY][tempX+1] != ' '){
            ballVelX = -1;
            bricks[tempY][tempX+1] = ' ';
        }
        else if(bricks[tempY+1][tempX] != ' '){
            ballVelY = -1;
            bricks[tempY+1][tempX] = ' ';
        }
        // else if(bricks[tempY][tempX] != ' '){
        //     ballVelX = -1*ballVelX;
        //     ballVelY = -1*ballVelY;
        //     bricks[tempY][tempX] = ' ';
        //     tempX = ballPosX;
        //     tempY = ballPosY;
        // }
        bricks[ballPosY][ballPosX] = ' ';
        ballPosX = tempX;
        ballPosY = tempY;
        bricks[ballPosY][ballPosX] = 'O';
        ball_last_time = chrono::high_resolution_clock::now();
    }
}

void updateReflector(){
    for(int i=0;i<col;i++){
        if(i>=refX && i<=refX+REF_LEN){
            bricks[row-1][i] = '=';
        }
        else{
            bricks[row-1][i] = ' ';
        }
    }
}

char** generateGrid(){
    char brickTypes[] = {'*','@','#','$','%','&', ' '};
    int btsize = 6;
    row = ws.ws_row;
    col = ws.ws_col;
    refX = col/2-2;
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
            }
        }
    }
    for(int i=row/2;i<row;i++){
        bricks[i] = new char[col];
        for(int j=0;j<col;j++){
            bricks[i][j] = ' ';
        }
    }
    for(int i=refX;i<=refX + REF_LEN;i++){
        bricks[row-1][i] = '=';
    }
    ballVelX = -1;
    ballVelY = -1;
    ballPosY = row-2;
    ballPosX = refX+2;
    bricks[ballPosY][ballPosX] = 'O';
    return bricks;
}

void renderGrid(){
    char *render = (char *)malloc(col);
    memcpy(render, bricks[0], col);
    int renderLen = col;
    for(int i=1;i<row;i++){
        char *temp = (char *)realloc(render, renderLen+col+5);
        memcpy(&temp[renderLen], "\x1b[K", 3);
        memcpy(&temp[renderLen+3], "\r\n", 2);
        memcpy(&temp[renderLen+5], bricks[i], col);
        renderLen += col+5;
        render = temp;
    }
    write(STDOUT_FILENO, render, renderLen);
    free(render);
}

void refreshScreen(){
    moveBall();
    renderGrid();
    write(STDOUT_FILENO, "\x1b[H", 3);
}

int main(){
    enableRawMode();
    getWindowSize();
    bricks = generateGrid();
    while(true){
        refreshScreen();
        processKeyPress();
    }
    return 0;
}