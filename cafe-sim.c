#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

int min(int a, int b)
{
    if (a < b)
    {
        return a;
    }
    return b;
}

#define MAX_COFFEE_TYPE 100
#define MAX_CUSTOMER 100
#define MAX_BARISTA 100

pthread_t customers[MAX_CUSTOMER];
pthread_t baristas[MAX_BARISTA];

pthread_mutex_t printlock;
pthread_mutex_t orderslock;
sem_t barista_sem;
sem_t orders_sem;
sem_t order_complete[MAX_CUSTOMER];
sem_t order_started[MAX_CUSTOMER];
sem_t wakeup[MAX_BARISTA];

int barista_status[MAX_BARISTA];

// pthread_mutex_t cust_create_lock;
// pthread_mutex_t cust_main_create_lock = PTHREAD_MUTEX_INITIALIZER;

int B, K, N;
char coffee_type[MAX_COFFEE_TYPE][100];
int prep_time[MAX_COFFEE_TYPE];
int customer_id[MAX_CUSTOMER];
char customer_coffee[MAX_CUSTOMER][100];
int arrival_time[MAX_CUSTOMER];
int tolerance_time[MAX_CUSTOMER];
int min_index_arr[MAX_BARISTA];

int order[MAX_CUSTOMER][4]; // 0: customer_id, 1: coffee_type, 2: prep_time, 3: tolerance_time

int waiting[MAX_CUSTOMER];
int waiting_time[MAX_CUSTOMER];
int wastage = 0;
int served = 0;
int orders;

void *barista_func(void *arg)
{
    int id = *(int *)arg;
    while (1)
    {
        // pthread_mutex_lock(&printlock);
        // // printf("Orders %d served %d in barista %d\n", orders, served, id);
        // pthread_mutex_unlock(&printlock);
        // printf("Barista %d created\n", id);
        if (orders <= served)
        {
            break;
        }

        if (barista_status[id - 1] == 0)
        {
            struct timespec t1;
            clock_gettime(CLOCK_REALTIME, &t1);
            t1.tv_sec += 1;
            while (sem_timedwait(&wakeup[id-1], &t1) == -1 && errno == EINTR)
            {
                continue;
            }
            if (errno == ETIMEDOUT)
            {
                continue;
            }
        }

        // int flag_break = 0;
        // for (int i = 0; i <id; i++)
        // {
        //     if (barista_status[i] == 0)
        //     {
        //         flag_break = 1;
        //         break;
        //     }
        // }
        // if (flag_break == 1)
        // {
        //     continue;
        // }
        // printf("Barista %d after sem_wait\n", id);

        // printf("Barista %d before sem_wait\n", id);
        sem_wait(&orders_sem);
        // printf("Barista %d after getting orders_sem\n", id);
        // usleep(10);
        // sem_wait(&wakeup[id]);
        // sem_wait(&mutex);
        // sleep(1);
        // usleep(id * 1000);
        pthread_mutex_lock(&orderslock);
        int min_index = -1;
        for (int i = 0; i < N; i++)
        {
            if (waiting[i] == 1)
            {
                min_index = i;
                break;
            }
        }
        // printf("Barista %d before min_index\n", id);
        if (min_index == -1)
        {
            // sem_post(&mutex);
            sem_post(&orders_sem);
            pthread_mutex_unlock(&orderslock);
            continue;
        }
        waiting[min_index] = 0;
        barista_status[id] = 1;
        sem_post(&wakeup[id]);
        pthread_mutex_unlock(&orderslock);

        sleep(1);
        pthread_mutex_lock(&orderslock);
        // When a barista b begins preparing the order of customer c, print [cyan colour] Barista b begins preparing the order of customer c at t second(s)
        pthread_mutex_lock(&printlock);
        printf("\033[0;36mBarista %d begins preparing the order of customer %d at %d second(s)\033[0m\n", id, order[min_index][0], arrival_time[min_index] + waiting_time[min_index] + 1);
        pthread_mutex_unlock(&printlock);
        // sem_post(&mutex);

        pthread_mutex_unlock(&orderslock);
        sleep(order[min_index][2]);
        // When a barista b successfully complete the order of customer c, print [blue colour] Barista b successfully completes the order of customer c
        if (waiting[min_index] == 0)
        {
            usleep(10);
            pthread_mutex_lock(&printlock);
            // When a barista b begins preparing the order of customer c, print [cyan colour] Barista b begins preparing the order of customer c at t second(s)
            printf("\033[0;34mBarista %d successfully completes the order of customer %d\033[0m\n", id, order[min_index][0]);
            sem_post(&order_complete[min_index]);
            pthread_mutex_unlock(&printlock);
            served++;
            sem_post(&orders_sem);
        }
        else
        {
            pthread_mutex_lock(&printlock);
            // When a barista b begins preparing the order of customer c, print [cyan colour] Barista b begins preparing the order of customer c at t second(s)
            printf("\033[0;34mBarista %d successfully completes the order of customer %d\033[0m\n", id, order[min_index][0]);
            // sem_post(&order_complete[min_index]);
            pthread_mutex_unlock(&printlock);
            wastage++;
            sem_post(&orders_sem);
        }
        barista_status[id] = 0;
        usleep(1000);
        sem_post(&barista_sem);
    }
    return NULL;
}

void *customer_func(void *arg)
{
    int id = *(int *)arg;
    pthread_mutex_lock(&orderslock);
    int customer_id = order[id][0];
    int coffee_type_id = order[id][1];
    int prep_time = order[id][2];
    int tolerance_time = order[id][3];
    pthread_mutex_unlock(&orderslock);
    sleep(arrival_time[id]);

    pthread_mutex_lock(&printlock);
    // When a customer c arrives, print [white colour] Customer c arrives at t second(s)
    printf("\033[0;37mCustomer %d arrives at %d second(s)\033[0m\n", customer_id, arrival_time[id]);
    // When a customer c places their order, print [yellow colour] Customer c orders an espresso
    printf("\033[0;33mCustomer %d orders an %s\033[0m\n", customer_id, coffee_type[coffee_type_id]);
    pthread_mutex_unlock(&printlock);

    // get current time

    struct timespec time1;
    clock_gettime(CLOCK_REALTIME, &time1);
    double t1 = time1.tv_sec * 1000 + time1.tv_nsec / 1000000;

    sem_wait(&order_started[customer_id - 1]);

    clock_gettime(CLOCK_REALTIME, &time1);
    double t2 = time1.tv_sec * 1000 + time1.tv_nsec / 1000000;

    int waiting_time1 = (int)(t2 - t1) / 1000;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += tolerance_time - waiting_time1;
    // struct timeval tv;
    // gettimeofday(&tv, NULL);

    // double begin = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    struct timespec ts2;
    clock_gettime(CLOCK_REALTIME, &ts2);
    double begin = ts2.tv_sec * 1000 + ts2.tv_nsec / 1000000;
    // double begin =time(NULL);

    while (sem_timedwait(&barista_sem, &ts) == -1 && errno == EINTR)
    {
        continue;
    }

    if (errno == ETIMEDOUT)
    {
        waiting_time[id] = tolerance_time - 1;
        pthread_mutex_lock(&printlock);
        // If a customer’s patience runs out, print [red colour] Customer c leaves without their order at t second(s)
        printf("\033[0;31mCustomer %d leaves without their order at %d second(s)\033[0m\n", customer_id, arrival_time[id] + tolerance_time);
        pthread_mutex_unlock(&printlock);
        pthread_mutex_lock(&orderslock);
        orders--;
        sem_post(&order_started[customer_id]);
        pthread_mutex_unlock(&orderslock);
        return NULL;
    }

    // gettimeofday(&tv, NULL);
    // double end = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    // double end = time(NULL);
    // printf("Waiting time of customer %d is %d\n", customer_id, waiting_time_sec);
    struct timespec ts3;
    clock_gettime(CLOCK_REALTIME, &ts3);
    double end = ts3.tv_sec * 1000 + ts3.tv_nsec / 1000000;

    int waiting_time_sec = (int)(end - begin) / 1000 + waiting_time1;
    pthread_mutex_lock(&orderslock);
    waiting_time[id] = waiting_time_sec;
    // sem_wait(&mutex);
    waiting[id] = 1;

    sem_post(&orders_sem);
    // printf("Customer %d has placed order\n", customer_id);
    sem_post(&order_started[customer_id]);
    pthread_mutex_unlock(&orderslock);
    // printf("Customer %d has placed order\n", customer_id);

    // struct timespec ts2;
    // clock_gettime(CLOCK_REALTIME, &ts2);
    // ts2.tv_sec +=
    int min_time = min(prep_time + 1, tolerance_time - waiting_time_sec);
    sleep(min_time);

    if (min_time == (prep_time + 1))
    {
        sem_wait(&order_complete[id]);
        pthread_mutex_lock(&printlock);
        // If a customer successfully gets their order, print [green colour] Customer c leaves with their order at t second(s)
        printf("\033[0;32mCustomer %d leaves with their order at %d second(s)\033[0m\n", customer_id, arrival_time[id] + waiting_time_sec + prep_time + 1);
        pthread_mutex_unlock(&printlock);
    }
    else
    {
        waiting[id] = 2;
        pthread_mutex_lock(&orderslock);
        pthread_mutex_lock(&printlock);
        // If a customer’s patience runs out, print [red colour] Customer c leaves without their order at t second(s)
        printf("\033[0;31mCustomer %d leaves without their order at %d second(s)\033[0m\n", customer_id, arrival_time[id] + tolerance_time);
        orders--;
        pthread_mutex_unlock(&printlock);
        pthread_mutex_unlock(&orderslock);
    }

    return NULL;
}

int main()
{
    scanf("%d %d %d", &B, &K, &N);
    for (int i = 0; i < K; i++)
    {
        scanf("%s %d", coffee_type[i], &prep_time[i]);
    }
    for (int i = 0; i < N; i++)
    {
        scanf("%d %s %d %d", &customer_id[i], customer_coffee[i], &arrival_time[i], &tolerance_time[i]);
        tolerance_time[i]++;
    }

    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < K; j++)
        {
            if (strcmp(customer_coffee[i], coffee_type[j]) == 0)
            {
                order[i][0] = customer_id[i];
                order[i][1] = j;
                order[i][2] = prep_time[j];
                order[i][3] = tolerance_time[i];
            }
        }
    }

    // for (int i = 0; i < N; i++)
    // {
    //     printf("%d %d %d %d\n", order[i][0], order[i][1], order[i][2], order[i][3]);
    // }

    pthread_mutex_init(&printlock, NULL);
    pthread_mutex_init(&orderslock, NULL);

    orders = N;

    sem_init(&barista_sem, 0, B);
    sem_init(&orders_sem, 0, 0);
    // sem_init(&mutex, 0, 1);

    for (int i = 0; i < B + 1; i++)
    {
        barista_status[i] = 0;
    }
    barista_status[0] = 1;

    for (int i = 0; i < B + 1; i++)
    {
        sem_init(&wakeup[i], 0, 0);
    }
    for (int i = 0; i < N + 1; i++)
    {
        sem_post(&wakeup[0]);
    }

    for (int i = 0; i < N + 1; i++)
    {
        sem_init(&order_complete[i], 0, 0);
    }
    for (int i = 0; i < N + 1; i++)
    {
        sem_init(&order_started[i], 0, 0);
    }
    sem_post(&order_started[0]);

    for (int i = 0; i < N + 1; i++)
    {
        waiting[i] = 0;
    }

    int barista_ids[B];
    for (int i = 0; i < B; i++)
    {
        barista_ids[i] = i + 1;
        // printf("Creating barista %d before thread\n", barista_ids[i]);
        if (pthread_create(baristas + i, NULL, &barista_func, (void *)&barista_ids[i]) != 0)
        {
            printf("Error creating barista %d\n", barista_ids[i]);
        }
        // printf("Creating barista %d after thread\n", barista_ids[i]);
    }

    int cust_ids[N];
    for (int i = 0; i < N; i++)
    {
        cust_ids[i] = i;
        // printf("Creating customer %d before thread\n", cust_ids[i]);
        if (pthread_create(customers + i, NULL, &customer_func, (void *)&cust_ids[i]) != 0)
        {
            printf("Error creating customer %d\n", cust_ids[i]);
        }
        // printf("Creating customer %d after thread\n", cust_ids[i]);
    }

    for (int i = 0; i < N; i++)
    {
        if (pthread_join(customers[i], NULL) != 0)
        {
            printf("Error joining customer %d\n", i);
        }
    }

    for (int i = 0; i < B; i++)
    {
        if (pthread_join(baristas[i], NULL) != 0)
        {
            printf("Error joining barista %d\n", i);
        }
    }

    for (int i = 0; i < B; i++)
    {
        sem_post(&barista_sem);
    }
    pthread_mutex_destroy(&printlock);
    pthread_mutex_destroy(&orderslock);

    printf("\n%d coffee wasted\n", wastage);

    double avg_waiting_time = 0;
    for (int i = 1; i < N + 1; i++)
    {
        avg_waiting_time += waiting_time[i];
    }
    avg_waiting_time /= N;
    printf("Average waiting time is %lf\n", avg_waiting_time);

    return 0;
}
