#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>


enum MSG_TYPE
{
    READY = 1,
    FINISH = 2,
    START = 3

};


struct mymsgbuf
{
    long type;
    int number;
};

struct mymsgbuf start = {START, 0};


int msqid = 0;
int length = sizeof(struct mymsgbuf) - sizeof(long);


double currentTime();
int judge(long num_of_runner);
int runner(int position, long num_of_runner);


int main(int argc, char* argv[]) {
    long num_of_runner = 20;

    if(argc != 2)
    {
        perror("Wrong number of arguments\n");
        return 0;
    }

    num_of_runner = strtol(argv[1], NULL, 0);

    if(num_of_runner <= 0)
    {
        perror("Incorrect number of runners\n");
        return 0;
    }

    if((msqid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT)) < 0)
    {
        perror("Can't get msqid\n");
        exit(-1);
    }

    pid_t pid = fork();

    if(pid == 0)
        judge(num_of_runner);

    for (int i = 1; i < num_of_runner + 1; i++)
    {
        if(pid)
        {
            pid = fork();
            if(pid == 0)
                runner(i, num_of_runner);
        }
    }
    if(pid) {
        for (int i = 0; i < num_of_runner + 1; i++)
            wait(NULL);
        msgctl(msqid, IPC_RMID, NULL);
        return 0;
    }
}


double currentTime()
{
    struct timeval time = {};
    gettimeofday(&time, NULL);
    return time.tv_sec + (double)time.tv_usec / 1000000;
}

int judge(long num_of_runner)
{
    printf("Judge came\n");
    for (int i = 0; i < num_of_runner; i++)
    {
        struct mymsgbuf ready;
        if (msgrcv(msqid, &ready, length, READY, 0) < 0)
        {
            perror("Can't receive message from queue\n");
            return 1;
        }
        printf("Runner %d is ready to start\n", ready.number);
    }

    printf("START!\n");

    double start_of_race = currentTime();

    struct mymsgbuf start = {START + 1, 0};
    if (msgsnd(msqid, &start, length, 0) < 0)
    {
        perror("Can't send message to queue\n");
        return 1;
    }
    struct mymsgbuf finish;
    if (msgrcv(msqid, &finish, length, FINISH, 0) < 0)
    {
        perror("Can't receive message from queue\n");
        return 1;
    }

    double finish_of_race = currentTime();
    printf("Total time: %.2lf\n", finish_of_race - start_of_race);
    return 0;


}


int runner(int position, long num_of_runner)
{
    printf("Runner %d came\n", position);

    struct mymsgbuf ready = {(long) READY, position};
    if (msgsnd(msqid, &ready, length, 0) < 0)
    {
        perror("Can't send message to queue\n");
        return 1;
    }

    struct mymsgbuf start;
    if (msgrcv(msqid, &start, length, START + position, 0) < 0)
    {
        perror("Can't receive message from queue\n");
        return 1;
    }
    printf("Runner %d start\n", position);

    usleep((rand() % 100) * 8000);

    printf("Runner %d finish\n", position);

    if (position != num_of_runner)
    {
        start.type = START + position + 1;
        if (msgsnd(msqid, &start, length, 0) < 0)
        {
            perror("Can't send message to queue\n");
            return 1;
        }
    }
    else
    {
        struct mymsgbuf finish = {FINISH, position};
        if (msgsnd(msqid, &finish, length, 0) < 0)
        {
            perror("Can't send message to queue\n");
            return 1;
        }
    }
    return 0;
}