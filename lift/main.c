#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

enum {
    CALL_EN = 0,
    CALL_EX,
    PASSENGERS,
    WAIT,
    DOOR
};
int semid;
int myshmid;

void elevator(long elevator_capacity, long number_level);

void passenger(int i, long number_flour);

int main(int argc, char *argv[]) {
    if (argc != 4) {
        perror("Wrong number of arguments\n");
        return 0;
    }

    long number_people = strtol(argv[1], NULL, 0);
    long elevator_capacity = strtol(argv[2], NULL, 0);
    long number_flour = strtol(argv[3], NULL, 0);

    if((semid = semget(IPC_PRIVATE, 4 + 2 * (int)number_flour, 0666 | IPC_CREAT)) < 0)
    {
        perror("Can't get semid\n");
        exit(-1);
    }


    key_t key;

    key = ftok("CMakeLists.txt", 0);
    if ((myshmid = shmget(key, (2 * number_flour + 2) * sizeof(int), 0666 | IPC_CREAT | IPC_EXCL)) < 0) {

        if (errno != EEXIST) {
            printf("Can\'t create shared memory\n");
            exit(-1);
        } else {

            if ((myshmid = shmget(key, (2 * number_flour + 2) * sizeof(int), 0)) < 0) {
                printf("Can't shared memory\n");
                exit(-1);
            }
        }
    }

    int* array;

    if ((array = (int *) shmat(myshmid, NULL, 0)) == (int *) (-1)) {
        printf("Can't attach shared memory\n");
        exit(-1);
    }
    array[0] = (int)number_people;  // counter of people on floors
    array[1] = 0; // counter of people in elevator

    for(int i = 2; i <= 2 * number_flour + 1; i++)
        array[i] = 0; // each floor has two counter for entry and exit

    struct sembuf init_button = {CALL_EN, 1, 0};
    semop(semid, &init_button, 1);

    struct sembuf init_button1 = {CALL_EX, 1, 0};
    semop(semid, &init_button1, 1);

    struct sembuf init_button2 = {PASSENGERS, 1, 0};
    semop(semid, &init_button2, 1);

    int pid = fork();
    if (pid == 0)
        elevator(elevator_capacity, number_flour);

    for (int i = 1; i < number_people + 1; i++) {
        if (fork() == 0)
            passenger(i, number_flour);
    }

    for (int i = 0; i < number_people + 1; i++)
        wait(NULL);


    semctl(semid, 0, IPC_RMID, NULL);
    shmctl(myshmid, IPC_RMID, NULL);
    return 0;
}

void passenger(int i, long number_flour) {

    srand(time(NULL));
    int my_flour = (rand() + i) % (int) number_flour + 1;
    int desired_flour = (rand() + i) % (int) number_flour + 1;
    int *array = NULL;

    if (my_flour == desired_flour)
        desired_flour = desired_flour % (int) number_flour + 1;

    if ((array = (int *) shmat(myshmid, NULL, 0)) == (int *) (-1)) {
        printf("Can't attach shared memory\n");
        exit(-1);
    }

    struct sembuf push_button = {CALL_EN, -1, 0};
    semop(semid, &push_button, 1);
    array[1 + my_flour]++;
    struct sembuf release_button = {CALL_EN, 1, 0};
    semop(semid, &release_button, 1);

    printf("Passenger %d pressed %d\n", i, my_flour);

    struct sembuf enter = {DOOR - 1 + my_flour, -1, 0};
    semop(semid, &enter, 1);

    struct sembuf wait = {WAIT, 1, 0};
    semop(semid, &wait, 1);

    printf("Passenger %d entered the elevator\n", i);

    struct sembuf push_button_ex = {CALL_EX, -1, 0};
    semop(semid, &push_button_ex, 1);
    array[1 + number_flour + desired_flour]++;
    struct sembuf release_button_ex = {CALL_EX, 1, 0};
    semop(semid, &release_button_ex, 1);

    printf("Passenger %d pressed %d\n", i, desired_flour);

    struct sembuf leave = {DOOR - 1 + number_flour + desired_flour, -1, 0};
    semop(semid, &leave, 1);

    semop(semid, &wait, 1);

    struct sembuf pass_check = {PASSENGERS, -1, 0};
    semop(semid, &pass_check, 1);
    array[0]--;
    struct sembuf pass_check_end = {PASSENGERS, 1, 0};
    semop(semid, &pass_check_end, 1);

    printf("Passenger %d reached the goal\n", i);

    if (shmdt(array) < 0) {
        printf("Can't detach shared memory\n");
        exit(-1);
    }
    exit(EXIT_SUCCESS);
}

void elevator(long elevator_capacity, long number_level) {
    int level = 1;
    int *array = NULL;
    int direction_flag = 1;
    if ((array = (int *) shmat(myshmid, NULL, 0)) == (int *) (-1)) {
        printf("Can't attach shared memory\n");
        exit(-1);
    }
    int counter_ex = 0, counter_en = 0;
    while (1) {
        sleep(1);
        printf("Elevator on %d flour\n", level);
        struct sembuf exit_check = {CALL_EX, -1, 0};
        semop(semid, &exit_check, 1);
        counter_ex = array[1 + number_level + level];
        struct sembuf exit_check_end = {CALL_EX, 1, 0};
        semop(semid, &exit_check_end, 1);

        if(counter_ex != 0)
        {
            struct sembuf exit = {DOOR - 1 + number_level + level, (short) counter_ex, 0};
            semop(semid, &exit, 1);

            struct sembuf wait = {WAIT, -(short)counter_ex, 0};
            semop(semid, &wait, 1);

            array[1] -= counter_ex;
        }

        semop(semid, &exit_check, 1);
        array[1 + number_level + level] = 0;
        semop(semid, &exit_check_end, 1);

        struct sembuf enter_check = {CALL_EN, -1, 0};
        semop(semid, &enter_check, 1);
        counter_en = array[1 + level];
        struct sembuf enter_check_end = {CALL_EN, 1, 0};
        semop(semid, &enter_check_end, 1);

        if(counter_en != 0 && (elevator_capacity - array[1]) > 0) {
            if (counter_en > (elevator_capacity - array[1]))
                counter_en = (int) (elevator_capacity - array[1]);
            struct sembuf enter = {DOOR - 1 + level, (short) counter_en, 0};
            semop(semid, &enter, 1);

            struct sembuf wait = {WAIT, -(short) counter_en, 0};
            semop(semid, &wait, 1);

            array[1] += counter_en;

            semop(semid, &enter_check, 1);
            array[1 + level] -= counter_en;
            semop(semid, &enter_check_end, 1);
        }
        if (level == 1 || level == number_level)
            direction_flag = !direction_flag;

        if (direction_flag)
            level--;
        else
            level++;

        struct sembuf pass_check = {PASSENGERS, -1, 0};
        semop(semid, &pass_check, 1);

        if(array[0] == 0)
            break;

        struct sembuf pass_check_end = {PASSENGERS, 1, 0};
        semop(semid, &pass_check_end, 1);
    }

    if (shmdt(array) < 0) {
        printf("Can't detach shared memory\n");
        exit(-1);
    }
    printf("Elevator finished\n");
    exit(EXIT_SUCCESS);
}