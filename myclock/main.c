#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

time_t time_st;
struct tm *time_info;
struct winsize Window;
char str_t[128] = {};

int x;
int y;

sigset_t mask;

void print_frame()
{
    printf("\033[2J");
    printf("\033[0;0f");

    for(int i = 0; i < y + 1; i++)
    {
        if(i == 0 || i == (y))
        {
            for (int k = 0; k < x; k++)
                fputs("*", stdout);
        }
        else
        {
            fputs("*", stdout);
            printf("\033[%d;%dH*\n", i, x);
        }
    }

}


void win_sz_handler(int sign_n)
{
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &Window);

    x = Window.ws_col;
    y = Window.ws_row;

    print_frame();
    sigprocmask(SIG_UNBLOCK, &mask, 0);
}

void alarm_handler(int sign_n)
{
    time(&time_st);
    time_info = localtime(&time_st);
    strftime (str_t, 128, "%X", time_info);
    int time_y = (y + 1)/ 2;
    int time_x = x / 2 - 4;
    printf("\033[%d;%dH%s\n", time_y, time_x, str_t);
    alarm(1);
}

int main() {

    sigaddset(&mask, SIGALRM);
    struct sigaction win = {};
    win.sa_mask = mask;
    win.sa_handler = &win_sz_handler;

    sigaction(SIGWINCH, &win, 0);
 //   signal(SIGWINCH, win_sz_handler);
    signal(SIGALRM, alarm_handler);

    sigaddset(&mask, SIGALRM);

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &Window);

    x = Window.ws_col;
    y = Window.ws_row;

    time(&time_st);
    time_info = localtime(&time_st);
    strftime (str_t, 128, "%X", time_info);

    print_frame();

    alarm(1);

    while(1)
    {
        pause();
    }

}
