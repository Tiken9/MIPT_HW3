#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <unistd.h>
#include <errno.h>

enum
{
    SHIP = 0,
    TRAP,
    REST,
    EXIT,
    WAIT
};
int semid;

void ship(long ship_capacity, long trap_capacity, long number_trips);
void passenger(int number);

int main(int argc, char* argv[])
{
    if(argc != 5)
    {
        perror("Wrong number of arguments\n");
        return 0;
    }

    long number_people = strtol(argv[1], NULL, 0);
    long ship_capacity = strtol(argv[2], NULL, 0);
    long trap_capacity = strtol(argv[3], NULL, 0);
    long number_trips = strtol(argv[4], NULL, 0);

    if(number_people < ship_capacity)
        ship_capacity = number_people;

    if((semid = semget(IPC_PRIVATE, 5, 0666 | IPC_CREAT)) < 0)
    {
        perror("Can't get semid\n");
        exit(-1);
    }
    int pid = fork();
    if(pid == 0)
        ship(ship_capacity, trap_capacity, number_trips);

    for(int i = 1; i < number_people + 1; i++)
    {
        if(fork() == 0)
            passenger(i);
    }

    for(int i = 0; i < number_people + 1; i++)
        wait(NULL);


    semctl(semid, 0, IPC_RMID, NULL);
    return 0;
}



void ship(long ship_capacity, long trap_capacity, long number_trips)
{
    printf("Ship arrived\n");

    struct sembuf begin = {WAIT, (short)ship_capacity, 0};
    semop(semid, &begin, 1);

    for(int i = 0; i < number_trips; i++)
    {
        struct sembuf init = {SHIP, (short)ship_capacity, 0};
        semop(semid, &init, 1);

        struct sembuf open_trap = {TRAP, (short) trap_capacity, 0};
        semop(semid, &open_trap, 1);
        printf("Trap open\n");

        struct sembuf wait_passengers = {WAIT, -(short)(2 * ship_capacity), 0};
        semop(semid, &wait_passengers, 1);

        struct sembuf close_trap = {TRAP, -(short)trap_capacity, 0};
        semop(semid, &close_trap, 1);
        printf("Trap closed\n");


        printf("Ship sailed\n");

        sleep(2);

        struct sembuf mooring = {REST, (short)ship_capacity, 0};
        semop(semid, &mooring, 1);
        printf("Ship moored\n");
    }

    struct sembuf init = {SHIP, (short)ship_capacity, 0};
    semop(semid, &init, 1);

    struct sembuf end_of_travel = {EXIT, 1, 0};
    semop(semid, &end_of_travel, 1);

    struct sembuf open_trap = {TRAP, (short)trap_capacity, 0};
    semop(semid, &open_trap, 1);
    printf("Trap open\n");

    struct sembuf wait_passengers = {WAIT, -(short)(1 * ship_capacity), 0};
    semop(semid, &wait_passengers, 1);

    struct sembuf close_trap = {TRAP, -(short)trap_capacity, 0};
    semop(semid, &close_trap, 1);

    printf("Trap closed\n");
    printf("The walk is over\n");

    exit(EXIT_SUCCESS);
}

void passenger(int number)
{
    while(1)
    {

        struct sembuf go_to_ship = {SHIP, -1, 0};
        semop(semid, &go_to_ship, 1);

        struct sembuf end_of_travel = {EXIT, 0, IPC_NOWAIT};
        int end = semop(semid, &end_of_travel, 1);
        if(end == -1 && errno == EAGAIN)
            break;

        printf("Passenger %d go to ship\n", number);

        struct sembuf go_to_trap = {TRAP, -1, 0};
        semop(semid, &go_to_trap, 1);
        printf("Passenger %d on the trap\n", number);

        struct sembuf on_trap = {WAIT, 1, 0};
        semop(semid, &on_trap, 1);

        usleep(10000);

        struct sembuf get_off_trap = {TRAP, 1, 0};
        semop(semid, &get_off_trap, 1);
        printf("Passenger %d on the ship\n", number);

        struct sembuf rest = {REST, -1, 0};
        semop(semid, &rest, 1);
        printf("Passenger %d rested\n", number);

        semop(semid, &go_to_trap, 1);
        printf("Passenger %d on the trap\n", number);

        semop(semid, &on_trap, 1);

        usleep(10000);

        semop(semid, &get_off_trap, 1);
        printf("Passenger %d on the shore\n", number);
    }

    struct sembuf give_others_disappointed = {SHIP, 1, 0};
    semop(semid, &give_others_disappointed, 1);

    printf("Passenger %d went away\n", number);
    exit(EXIT_SUCCESS);
}