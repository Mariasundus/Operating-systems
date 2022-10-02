#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/types.h>

// EOPSY 2022L Lab5 - Process Synchronization 
// To compile: 
//  gcc dp.c -o dp

// The Dining Philosophers

// The Dining Philosophers problem is a classic multi-process synchronization
// problem. The problem consists of five philosophers sitting at a table who do
// nothing but think and eat. Between each philosopher, there is a single fork
// In order to eat, a philosopher must have both forks. A problem can arise if
// each philosopher grabs the fork on the right, then waits for the fork on the
// left. In this case a deadlock has occurred, and all philosophers will starve.
// Also, the philosophers should be fair. Each philosopher should be able to eat
// as much as the rest.

// Implement in the C language the dining philosophers program. For
// synchronization implement and use the two following functions:

// void grab_forks( int left_fork_id );

// void put_away_forks( int left_fork_id );

// where parameters are integers identifying semaphores associated with forks.
// grab_forks() and put_away_forks() should use IPC semaphores (man semget, man
// semop) to make atomic changes on two semaphores at the same time. Print on the
// standard output verbose messages from philosophers.


const int N_PHILOSOPHERS =  5; // Number of philosophers
int semid; // Store semaphore set id
int semid2; // Store second semaphore for tracking total meals
int interrupt_occurrence = 0; // interrupt placeholder

void interrupt_handler() {
	interrupt_occurrence = 1;
}

void thinking(int p_num) {
    printf("Philosopher %d is thinking...Hmmm\n", p_num);
    sleep(1);
}

void grab_forks(int left_fork_id) {
    // initialize sembuf structure named sops which specifies an operation to be performed on a single semaphore
    // members of each element of this structure include: sem_num, sem_op, sem_flg
    struct sembuf sops[2]; 
    int right_fork_id = (left_fork_id + 1) % N_PHILOSOPHERS;

    sops[0].sem_num = left_fork_id; // semaphore number
    sops[0].sem_op = -1; // semaphore operation: less than 0 means abs value of sem_op is subtracted from semval
    sops[0].sem_flg = 0; // operation flags

    sops[1].sem_num = right_fork_id;
    sops[1].sem_op = -1;
    sops[1].sem_flg = 0;

    // int semop(int semid, struct sembuf *sops, size_t nsops);
    // perform operations on semaphores in the set indicated by semid
    // nsop specifies length of the array - this is the maximum number of operations allowed by a single semop() call
    if (semop(semid, sops, 2) == -1) {
        printf("Philosopher %d error grabbing forks\n", left_fork_id);
        exit(1);
    }

    printf("Philosopher %d is grabbing left fork %d and right fork %d\n", left_fork_id, left_fork_id, right_fork_id);
}

void eating(int p_num) {
    printf("Philosopher %d is eating...Yummy\n", p_num);
    sleep(1);
}

void put_away_forks(int left_fork_id) {
    struct sembuf sops[2]; 
    int right_fork_id = (left_fork_id + 1) % N_PHILOSOPHERS;

    sops[0].sem_num = left_fork_id; 
    sops[0].sem_op = 1; // semaphore operation: greater than 0 means we add value of sem_op to semval
    sops[0].sem_flg = 0; 

    sops[1].sem_num = right_fork_id;
    sops[1].sem_op = 1;
    sops[1].sem_flg = 0;

    if (semop(semid, sops, 2) == -1) {
        printf("Philosopher %d error putting away forks\n", left_fork_id);
        exit(1);
    }

    printf("Philosopher %d is putting away left fork %d and right fork %d\n", left_fork_id, left_fork_id, right_fork_id);    
}

void philosopher(int p_num) {
    // Check for fairness
    // Initialize number of meals for each philosopher
    int num_meals = 0;
    while(interrupt_occurrence == 0) {
        thinking(p_num);
        int total_meals = semctl(semid2, 0, GETVAL); // get value of total meal semaphore
        int avg_meals = total_meals / N_PHILOSOPHERS; // calculate average meals eaten from all philosophers
        printf("Philosopher %d: Number of meals: %d, Avg meals: %d\n", p_num, num_meals, avg_meals);
        // Check if number of meals eaten from this philosopher is less than or equal to the average number of meals
        if (num_meals <= avg_meals) {
            // Philosopher is starving, so they must get a chance to eat
            grab_forks(p_num);
            eating(p_num);
            num_meals++;
            
            // Increase global meal count
            struct sembuf sops;
            sops.sem_num = 0;
            sops.sem_op = 1;
            sops.sem_flg = 0;
            if (semop(semid2, &sops, 1) == -1 ) {
                 printf("Error increasing global meal count.\n");
                 exit(1);
            }

            put_away_forks(p_num);
        } else {
            sleep(2);
        }
    }
    // **Original version**
    // No fairness here
    // while(interrupt_occurrence == 0) {
    //     thinking(p_num);
    //     grab_forks(p_num);
    //     eating(p_num);
    //     put_away_forks(p_num);
    // }
}

int main() {
    signal(SIGINT, interrupt_handler); // define our own interrupt handler

    // initialize semaphore set and return set identifier to semid
    // a new set of nsems (N_PHILOSOPHERS) semaphores is created if key has value IPC_PRIVATE and IPC_CREAT is specified in semflg
    semid = semget(IPC_PRIVATE, N_PHILOSOPHERS, 0200 | IPC_CREAT); // 0200 sets write permissions for user and nothing for groups/other
    semid2 = semget(IPC_PRIVATE, 1, 0600 | IPC_CREAT); // second set of semaphors for checking average eaten
    
    // Check if semaphore set was created successfully
    if (semid == -1) {
        perror("Failed to create semaphores");
        exit(1);
    }

    // initialize each semaphore to 1 using semctl
    for (int i = 0; i < N_PHILOSOPHERS; i++) {
        int j = semctl(semid, i, SETVAL, 1);
        // check if semval was set successfully
        if (j == -1) {
            perror("Failed to set semaphore value");
            exit(1);
        }
    }

    // create philosophers
    for (int i = 0; i < N_PHILOSOPHERS; i++) {
        pid_t pid = fork();

        if (pid < 0) {
			printf("Child[%d]: not created correctly.\n", (int) getpid());
			kill(0, SIGTERM); // int kill(pid_t pid, int sig)
			exit(1);
		}
        // if pid == 0, we are in child process
        if (pid == 0) { 
            philosopher(i);
            exit(0);
        }
    }

    // waiting for processes to terminate
    int child_process_count = 0;
	while (wait(NULL) >= 0) {
		child_process_count++;
	}
	printf("Parent[%d]: %d child processes terminated.\n", (int) getpid(), child_process_count);

    return 0;
}
