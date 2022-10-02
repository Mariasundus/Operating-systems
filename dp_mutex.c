#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

// EOPSY Lab7 - Dining Philosopher's using Mutexes and Threads
// To compile: gcc -pthread dp_mutex.c -o dp_mutex

#define N 5
#define LEFT ( i + N - 1 ) % N
#define RIGHT ( i + 1 ) % N

#define THINKING 1
#define HUNGRY 2
#define EATING 3

pthread_t philosophers[N]; // declare threads to mimic philosophers
pthread_mutex_t m; // declare mutex m for general locking/unlocking threads
pthread_mutex_t s[N]; // declare mutex for forks
int state[N]; // Keeps philosopher states
int num_of_meals[N]; // stores number of meals for each philosopher
int total_meals; // keep track of total meals of all philosophers
void* philosopher(void* p_num);
void grab_forks(int i);
void put_away_forks(int i);
void test(int i);
void thinking(int p_num);
void eating(int p_num);

int interrupt_occurrence = 0; // interrupt flag
void interrupt_handler() {
	interrupt_occurrence = 1;
}

// Thread start routine
void* philosopher(void* p_num) {
    int id = *(int*)p_num; // we have to cast void pointer to int
    while(interrupt_occurrence == 0) {
        thinking(id);
        total_meals = 0;
        for (int i = 0; i < N; i++) {
            total_meals += num_of_meals[i]; // get total meals
        }
        int avg_meals = total_meals / N; // calculate avg meals for fairness
        printf("Philosopher %d: Number of meals: %d, Avg meals: %d\n", id, num_of_meals[id], avg_meals);
        if (num_of_meals[id] <= avg_meals) {
            // Philosopher is starving, so they must get a chance to eat
            grab_forks(id);
            eating(id);
            put_away_forks(id);
            num_of_meals[id]++;
            printf("Philosopher %d finished eating.\n", id);
        } else {
            sleep(1);
        }
    }
    free(p_num); // free allocated memory
}

void grab_forks(int i) {
    pthread_mutex_lock(&m);
    state[i] = HUNGRY;
    printf("Philosopher %d is hungry.\n", i);
    test(i);
    pthread_mutex_unlock(&m);
    pthread_mutex_lock(&s[i]);
}

void put_away_forks(int i) {
    pthread_mutex_lock(&m);
    state[i] = THINKING;
    test(LEFT);
    test(RIGHT);
    pthread_mutex_unlock(&m);
}

void test(int i) { // checking that a specific data can by changed by one thread at a time
    if (state[i] == HUNGRY && state[LEFT] != EATING && state[RIGHT] != EATING) {
        
        
        state[i] = EATING;
        pthread_mutex_unlock(&s[i]);
    } 
}

void thinking(int p_num) {
    printf("Philosopher %d is thinking...Hmmm\n", p_num);
    //sleep(1);
}

void eating(int p_num) {
    printf("Philosopher %d is eating...Yummy\n", p_num);
    //sleep(1);
}

int main(int argc, char* argv[]) {
    signal(SIGINT, interrupt_handler); // use our own interrupt handler
    // Initialize mutexes
    pthread_mutex_init(&m, NULL);
    // Initialize to 0's - locked
    pthread_mutex_init(&s[N], NULL);
    for (int i = 0; i < N; i++) {
        pthread_mutex_lock(&s[i]);
    }
    // Initiate philosopher states to thinking
    for (int i = 0; i < N; i++) {
        state[i] = THINKING;
    }
    // Create threads (philosophers)
    for (int i = 0; i < N; i++) {
        int *id = malloc(sizeof(int)); // dynamically allocate memory for i to pass to pthread_create()
        *id = i;
        if (pthread_create(&philosophers[i], NULL, &philosopher, id) != 0) {
            perror("Failed creating thread");
            exit(1);
        }
    }
    // Wait for threads to terminate
    for (int i = 0; i < N; i++) {
        if (pthread_join(philosophers[i], NULL) != 0) {
            perror("Failed joining thread");
            exit(1);
        }
    }
    // Destroy mutexes
    pthread_mutex_destroy(&m);
    pthread_mutex_destroy(&s[N]);
    // Show number of meals each philosopher ate
    for (int i = 0; i < N; i++) {
        printf("Philosopher %d: ate %d meals.\n", i, num_of_meals[i]);
    }
    return 0;
}
