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
#define REF_LEN 4
using namespace std;

void updateGrid();

struct termios orgTermios;
struct winsize ws;
int row;
int col;
char **bricks;
int refX;

void error(const char *msg){
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(msg);
    exit(1);
}

void disableRawMode(){
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &orgTermios) == -1)
        error("tcsetattr error");
    //enable cursor
    write(STDOUT_FILENO, "\e[?25h", 6);
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
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &rawTermios) == -1)
        error("tcsetattr error");
    //hide cursor
    write(STDOUT_FILENO, "\e[?25l", 6);
}

void refershScreen(){
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
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
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
        case 'a':
            refX--;
            refX = (refX<0?0:refX);
            updateGrid();
            break;
        case 'd':
            refX++;
            refX = (refX+REF_LEN>col?col-REF_LEN:refX);
            updateGrid();
            break;
    }
}

void getWindowSize(){
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
        error("error getting window size");
    }
}

void updateGrid(){
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
    return bricks;
}

void renderGrid(){
    char *render = (char *)malloc(col);
    memcpy(render, bricks[0], col);
    int renderLen = col;
    for(int i=1;i<row;i++){
        char *temp = (char *)realloc(render, renderLen+col+2);
        memcpy(&temp[renderLen], "\r\n", 2);
        memcpy(&temp[renderLen+2], bricks[i], col);
        renderLen += col+2;
        render = temp;
    }
    write(STDOUT_FILENO, render, renderLen);
    free(render);
}

int main(){
    enableRawMode();
    getWindowSize();
    bricks = generateGrid();
    while(true){
        refershScreen();
        renderGrid();
        processKeyPress();
    }
    return 0;
}