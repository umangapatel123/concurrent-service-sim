#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/time.h>

#define MAX_MACHINE 1000
#define MAX_CUSTOMER 1000
#define MAX_TOPPING 1000
#define MAX_FLAVOR 1000
#define MAX_ORDER 1000

int n, k, f, t;

pthread_t machine_thread[MAX_MACHINE];
pthread_t customer_thread[MAX_CUSTOMER];

pthread_mutex_t orderslock;
pthread_mutex_t printlock;

int waiting[MAX_CUSTOMER][MAX_ORDER];

sem_t customer_order_complete[MAX_CUSTOMER];
sem_t machine_order;

int ingrediants_shortage[MAX_CUSTOMER];
int machine_busy[MAX_CUSTOMER];

int current_capacity = 0;
int left_due_to_capacity = 0;
struct machine
{
    int id;
    int start_time;
    int end_time;
} machine[MAX_MACHINE];

struct flavor
{
    char name[100];
    int time;
} flavor[MAX_FLAVOR];

struct topping
{
    char name[100];
    int quantity;
} topping[MAX_TOPPING];

struct order
{
    char flavor[100];
    char topping[MAX_TOPPING][100];
} order;

struct customer_orders
{
    int orders_no;
    int flavor[MAX_ORDER];
    int topping[MAX_ORDER][MAX_TOPPING];
    int no_of_toppings[MAX_ORDER];
} customer_orders;

struct customer
{
    int id;
    int arrival_time;
    int no_of_icreams;
    struct order order[100];
    struct customer_orders customer_orders;
} customer[MAX_CUSTOMER];

void *machine_thread_function(void *arg)
{
    struct machine *m = (struct machine *)arg;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    time_t machine_start = ts.tv_sec;
    sleep(m->start_time);

    // When a machine m starts working, print [orange colour] Machine m has started working at t second(s)
    pthread_mutex_lock(&printlock);
    // use this orange shade \e[38;2;255;85;0m
    printf("\e[38;2;255;85;0mMachine %d has started working at %d second(s)\033[0m\n", m->id, m->start_time);
    pthread_mutex_unlock(&printlock);

    clock_gettime(CLOCK_REALTIME, &ts);
    time_t start_time = ts.tv_sec;
    time_t end_time = start_time + (m->end_time - m->start_time);
    int flag = 0;

    while (1)
    {
        clock_gettime(CLOCK_REALTIME, &ts);
        time_t current_time = ts.tv_sec;
        if (current_time >= end_time)
        {
            usleep(1000);
            pthread_mutex_lock(&printlock);
            // When a machine m stops working, print [orange colour] Machine m has stopped working at t second(s)
            printf("\e[38;2;255;85;0mMachine %d has stopped working at %d second(s)\033[0m\n", m->id, m->end_time);
            pthread_mutex_unlock(&printlock);
            if (m->id == n)
            {
                for (int i = 0; i < k + 1; i++)
                {
                    machine_busy[i] = 1;
                    for (int j = 0; j < MAX_ORDER; j++)
                    {
                        sem_post(&customer_order_complete[i]);
                    }
                }
            }
            break;
        }
        pthread_mutex_lock(&orderslock);
        if (current_capacity == k)
        {
            pthread_mutex_unlock(&orderslock);
            continue;
        }
        pthread_mutex_unlock(&orderslock);

        time_t rem_time = end_time - current_time;
        // use timedwait
        usleep(m->id * 1000);
        struct timespec ts1;
        clock_gettime(CLOCK_REALTIME, &ts1);
        ts1.tv_sec += rem_time;
        while (sem_timedwait(&machine_order, &ts1) == -1 && errno == EINTR)
            continue;

        if (errno == ETIMEDOUT)
        {
            continue;
        }

        usleep(m->id * 1000);

        pthread_mutex_lock(&orderslock);
        int customer_id = -1;
        int order_id = -1;
        int total_orders;
        int prep_time;
        clock_gettime(CLOCK_REALTIME, &ts);
        time_t now_inloop=ts.tv_sec;

        for (int i = 0; i < MAX_CUSTOMER; i++)
        {
            for (int j = 0; j < MAX_ORDER; j++)
            {
                if (waiting[i][j] == 1)
                {
                    customer_id = i;
                    order_id = j;
                    total_orders = customer[customer_id - 1].no_of_icreams;
                    prep_time=flavor[customer[customer_id - 1].customer_orders.flavor[order_id]].time;
                    if (ingrediants_shortage[customer_id] == 1)
                    {
                        for (int i = 0; i < total_orders; i++)
                        {
                            waiting[customer_id][i] = 0;
                            sem_post(&customer_order_complete[customer_id]);
                        }
                        customer_id = -1;
                        order_id = -1;
                        sem_post(&machine_order);
                        continue;
                    }
                    else if((now_inloop+prep_time) > end_time)
                    {
                        waiting[customer_id][order_id] = 1;
                        customer_id = -1;
                        order_id = -1;
                        sem_post(&machine_order);
                        continue;
                    }
                    break;
                }
            }
            if (customer_id != -1)
            {
                break;
            }
        }
        if (customer_id == -1)
        {
            sem_post(&machine_order);
            pthread_mutex_unlock(&orderslock);
            continue;
        }
        waiting[customer_id][order_id] = 0;

        clock_gettime(CLOCK_REALTIME, &ts);
        time_t now = ts.tv_sec;
        time_t end = now + prep_time;
        // check if machine will be free before end time
        if (end > end_time)
        {
            waiting[customer_id][order_id] = 1;
            sem_post(&machine_order);
            pthread_mutex_unlock(&orderslock);
            continue;
        }

        int flag = 0;
        int required_toppings[MAX_TOPPING];
        for (int i = 0; i < MAX_TOPPING; i++)
        {
            required_toppings[i] = 0;
        }
        for (int i = 0; i < customer[customer_id - 1].customer_orders.no_of_toppings[order_id]; i++)
        {
            // if (topping[customer[customer_id - 1].customer_orders.topping[order_id][i]].quantity == -1)
            // {
            //     continue;
            // }
            // if (topping[customer[customer_id - 1].customer_orders.topping[order_id][i]].quantity == 0)
            // {
            //     flag = 1;
            //     break;
            // }
            required_toppings[customer[customer_id - 1].customer_orders.topping[order_id][i]]++;
        }
        for (int i = 0; i < MAX_TOPPING; i++)
        {
            if (topping[i].quantity == -1)
            {
                continue;
            }
            if (topping[i].quantity < required_toppings[i])
            {
                flag = 1;
                break;
            }
        }
        if (flag == 1)
        {
            ingrediants_shortage[customer_id] = 1;
            for (int i = 0; i < total_orders; i++)
            {
                waiting[customer_id][i] = 0;
                sem_post(&customer_order_complete[customer_id]);
            }
            sem_post(&machine_order);
            pthread_mutex_unlock(&orderslock);
            continue;
        }
        for (int i = 0; i < customer[customer_id - 1].customer_orders.no_of_toppings[order_id]; i++)
        {
            if (topping[customer[customer_id - 1].customer_orders.topping[order_id][i]].quantity == -1)
            {
                continue;
            }
            topping[customer[customer_id - 1].customer_orders.topping[order_id][i]].quantity--;
        }
        pthread_mutex_unlock(&orderslock);

        pthread_mutex_lock(&printlock);
        // When a machine m is starts preparing the order of customer c, print [cyan colour] Machine m starts preparing ice cream 1 of customer c at t seconds(s)
        printf("\033[0;36mMachine %d starts preparing ice cream %d of customer %d at %ld second(s)\033[0m\n", m->id, order_id + 1, customer_id, now - machine_start);
        pthread_mutex_unlock(&printlock);
        // printf("prep time %d\n",prep_time);
        sleep(prep_time);

        pthread_mutex_lock(&printlock);
        // When a machine m is completes preparing the order of customer c, print [blue colour] Machine m completes preparing ice cream 1 of customer c at t seconds(s)
        printf("\033[0;34mMachine %d completes preparing ice cream %d of customer %d at %ld second(s)\033[0m\n", m->id, order_id + 1, customer_id, now - machine_start + prep_time);
        sem_post(&customer_order_complete[customer_id]);
        pthread_mutex_unlock(&printlock);

        // pthread_mutex_lock(&orderslock);
        // pthread_mutex_unlock(&orderslock);
    }
    return NULL;
}

void *customer_thread_function(void *arg)
{
    struct customer *c = (struct customer *)arg;
    sleep(c->arrival_time);

    if (current_capacity == 0)
    {
        left_due_to_capacity++;
        return NULL;
    }
    pthread_mutex_lock(&orderslock);
    current_capacity--;
    pthread_mutex_unlock(&orderslock);

    pthread_mutex_lock(&printlock);
    printf("\033[0;37mCustomer %d enters at %d second(s)\033[0m\n", c->id, c->arrival_time);
    printf("\033[0;33mCustomer %d orders %d ice creams\033[0m\n", c->id, c->no_of_icreams);
    for (int i = 0; i < c->no_of_icreams; i++)
    {
        printf("\e[0;33mIce cream %d: %s ", i + 1, c->order[i].flavor);
        int k = 0;
        while (c->order[i].topping[k][0] != '\0')
        {
            printf("%s ", c->order[i].topping[k]);
            k++;
        }
        printf("\033[0m\n");
    }
    pthread_mutex_unlock(&printlock);

    // check if proper ingrediants are available
    int required_toppings[MAX_TOPPING];
    for (int i = 0; i < MAX_TOPPING; i++)
    {
        required_toppings[i] = 0;
    }
    int flag = 0;
    for (int i = 0; i < c->no_of_icreams; i++)
    {
        for (int j = 0; j < c->customer_orders.no_of_toppings[i]; j++)
        {
            // if (topping[c->customer_orders.topping[i][j]].quantity == -1)
            // {
            //     continue;
            // }
            // if (topping[c->customer_orders.topping[i][j]].quantity == 0)
            // {
            //     flag = 1;
            //     break;
            // }
            required_toppings[c->customer_orders.topping[i][j]]++;
        }
    }
    for (int i = 0; i < MAX_TOPPING; i++)
    {
        if (topping[i].quantity == -1)
        {
            continue;
        }
        if (topping[i].quantity < required_toppings[i])
        {
            flag = 1;
            break;
        }
    }

    if (flag == 1)
    {
        ingrediants_shortage[c->id] = 1;
    }

    for (int i = 0; i < c->no_of_icreams; i++)
    {
        waiting[c->id][i] = 1;
    }

    // int total_prep_time = 0;
    // for (int i = 0; i < c->no_of_icreams; i++)
    // {
    //     total_prep_time += flavor[temp_orders.flavor[i]].time;
    // }

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    int start_time = ts.tv_sec;

    sleep(1);

    for (int i = 0; i < c->no_of_icreams; i++)
    {
        sem_post(&machine_order);
    }

    for (int i = 0; i < c->no_of_icreams; i++)
    {
        sem_wait(&customer_order_complete[c->id]);
    }

    clock_gettime(CLOCK_REALTIME, &ts);
    int end_time = ts.tv_sec;

    int total_prep_time = end_time - start_time;

    usleep(100);
    pthread_mutex_lock(&orderslock);
    if (ingrediants_shortage[c->id] == 1)
    {
        pthread_mutex_lock(&printlock);
        // If a customer was rejected due to ingredient shortage, print [red colour] Customer c left at t second(s) with an unfulfilled order
        printf("\033[0;31mCustomer %d left at %d second(s) with an unfulfilled order\033[0m\n", c->id, c->arrival_time + total_prep_time);
        pthread_mutex_unlock(&printlock);
    }
    else if (machine_busy[c->id] == 1)
    {
        // If a customer c was not serviced even when the parlour is closing (last machine has stopped working), print [red colour] Customer c was not serviced due to unavailability of machines
        pthread_mutex_lock(&printlock);
        printf("\033[0;31mCustomer %d was not serviced due to unavailability of machines\033[0m\n", c->id);
        pthread_mutex_unlock(&printlock);
    }
    else
    {
        // When a customer’s order is complete, as they will pick it up immidiately, print [green colour] Customer c has collected their order(s) and left at t second(s)
        pthread_mutex_lock(&printlock);
        printf("\033[0;32mCustomer %d has collected their order(s) and left at %d second(s)\033[0m\n", c->id, c->arrival_time + total_prep_time);
        pthread_mutex_unlock(&printlock);
    }
    current_capacity++;
    pthread_mutex_unlock(&orderslock);
    return NULL;
}

int main()
{

    scanf("%d %d %d %d", &n, &k, &f, &t);
    current_capacity = k;
    for (int i = 0; i < n; i++)
    {
        scanf("%d %d", &machine[i].start_time, &machine[i].end_time);
        machine[i].id = i + 1;
    }
    for (int i = 0; i < f; i++)
    {
        scanf("%s %d", flavor[i].name, &flavor[i].time);
    }
    for (int i = 0; i < t; i++)
    {
        scanf("%s %d", topping[i].name, &topping[i].quantity);
    }

    int i = 0;
    char c1;
    scanf("%c", &c1);
    int customer_count = 0;
    // printf("Enter the customer details\n");
    // printf("Enter Total number of customers\n");
    // scanf("%d", &customer_count);
    // printf("%d\n",customer_count);
    while (1)
    {
        // if (i == customer_count)
        //     break;
        char c;
        scanf("%c", &c);
        if (c == '\n')
            break;

        ungetc(c, stdin);

        scanf("%d %d %d", &customer[i].id, &customer[i].arrival_time, &customer[i].no_of_icreams);
        // printf("%d %d %d\n",customer[i].id,customer[i].arrival_time,customer[i].no_of_icreams);
        for (int j = 0; j < customer[i].no_of_icreams; j++)
        {
            scanf("%s", customer[i].order[j].flavor);
            // printf("%s ",customer[i].order[j].flavor);
            int k = 0;
            char temp[10000];
            fgets(temp, 10000, stdin);
            char *token = strtok(temp, " ");
            while (token != NULL)
            {
                strcpy(customer[i].order[j].topping[k], token);
                token = strtok(NULL, " ");
                k++;
            }
            customer[i].order[j].topping[k - 1][strlen(customer[i].order[j].topping[k - 1]) - 1] = '\0';
            customer[i].order[j].topping[k][0] = '\0';
        }
        i++;
    }

    customer_count = i;

    for (int i = 0; i < customer_count; i++)
    {
        for (int j = 0; j < customer[i].no_of_icreams; j++)
        {
            for (int k = 0; k < f; k++)
            {
                if (strcmp(customer[i].order[j].flavor, flavor[k].name) == 0)
                {
                    customer[i].customer_orders.flavor[j] = k;
                    break;
                }
            }
            int l = 0;
            while (customer[i].order[j].topping[l][0] != '\0')
            {
                for (int k = 0; k < t; k++)
                {
                    if (strcmp(customer[i].order[j].topping[l], topping[k].name) == 0)
                    {
                        customer[i].customer_orders.topping[j][l] = k;
                        customer[i].customer_orders.no_of_toppings[j]++;
                        break;
                    }
                }
                l++;
            }
        }
    }

    for (int i = 0; i < MAX_CUSTOMER; i++)
    {
        ingrediants_shortage[i] = 0;
    }

    for (int i = 0; i < MAX_CUSTOMER; i++)
    {
        for (int j = 0; j < MAX_ORDER; j++)
        {
            waiting[i][j] = 0;
        }
    }

    for (int i = 0; i < MAX_CUSTOMER; i++)
    {
        machine_busy[i] = 0;
    }

    for (int i = 0; i < MAX_CUSTOMER; i++)
    {
        sem_init(&customer_order_complete[i], 0, 0);
    }
    sem_init(&machine_order, 0, 0);

    pthread_mutex_init(&orderslock, NULL);
    pthread_mutex_init(&printlock, NULL);

    for (int i = 0; i < n; i++)
    {
        pthread_create(&machine_thread[i], NULL, machine_thread_function, (void *)&machine[i]);
    }
    for (int i = 0; i < customer_count; i++)
    {
        pthread_create(&customer_thread[i], NULL, customer_thread_function, (void *)&customer[i]);
    }
    for (int i = 0; i < n; i++)
    {
        pthread_join(machine_thread[i], NULL);
    }
    for (int i = 0; i < customer_count; i++)
    {
        pthread_join(customer_thread[i], NULL);
    }

    printf("Parlour Closed\n");
    printf("No. of customers that left due to capacity constraint: %d\n", left_due_to_capacity);

    return 0;
}
