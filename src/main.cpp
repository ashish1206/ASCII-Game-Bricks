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
using namespace std;

struct termios orgTermios;
struct winsize ws;
int row;
int col;
char **bricks;

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
    }
}

void getWindowSize(){
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
        error("error getting window size");
    }
}

char** generateBricks(){
    char brickTypes[] = {'*','@','#','$','%','&', ' '};
    int btsize = 6;
    row = ws.ws_row/2;
    col = ws.ws_col;
    char **bricks = new char*[row];
    srand(time(0));
    for(int i=0;i<row;i++){
        bricks[i] = new char[col];
        if(i==row/2)btsize++;
        for(int j=0;j<col;j++){
            if(i>0 && bricks[i-1][j] == ' '){
                bricks[i][j] = ' ';
            }
            else{
                bricks[i][j] = brickTypes[(int)(rand()%btsize)];
            }
        }
    }
    return bricks;
}

void renderBricks(){
    char *render = bricks[0];
    int renderLen = col;
    for(int i=1;i<row;i++){
        char *temp = (char *)realloc(render, renderLen+col+2);
        memcpy(&temp[renderLen], "\r\n", 2);
        memcpy(&temp[renderLen+2], bricks[i], col);
        renderLen += col+2;
        render = temp;
    }
    write(STDOUT_FILENO, render, renderLen);
}

int main(){
    enableRawMode();
    getWindowSize();
    bricks = generateBricks();
    while(true){
        refershScreen();
        renderBricks();
        processKeyPress();
    }
    return 0;
}