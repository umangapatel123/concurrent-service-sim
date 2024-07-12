# Concurrent Service Simulator

The Cafe and Ice Cream Parlor Simulator project implements multi-threaded simulations of food service environments. In the Cafe Simulator, customers arrive with specific coffee orders and patience thresholds. The system manages order placement, assigns available baristas, simulates coffee preparation considering different brewing times, and handles order completion or customer departures due to impatience. For the Ice Cream Parlor, the simulation tracks customer entry, processes multi-ice cream orders, checks ingredient availability, allocates machines within their operational hours, and manages the preparation and topping process. Both simulations provide detailed, color-coded output for each step, from customer arrival to order completion or rejection. The project addresses concurrent resource management, handles time constraints and limited ingredients, and includes analysis of system performance such as waiting times, wastage, and service optimization. Throughout both simulations, the implementation focuses on thread-safe operations using semaphores and mutex locks to manage shared resources effectively.

## Cafe Sim

### Implementation

- For the implementation of the cafe sim, I have used the concept of multithreading, mutex locks and semaphores.
- First, I have created a thread for each customer and each barista.
- I have created a mutex lock for `order processing(orderslock)` and `printing details(printlock)`.
- There are two semaphores, one for `available baristas(barista_sem)` and one for `available orders(orders_sem)`. The semaphore `barista_sem` is initialized with the number of baristas and `orders_sem` is initialized with 0.
- To always process minimum order number first, I have used semaphore array `order_started` of size `MAX_CUSTOMER` and initialized it with 0. When a customer thread starts, it waits for the semaphore `order_started` of its previous order number to be 1. When the barista thread processes an order, it sets the semaphore `order_started` of that order number to 1.
- When a previous order is processed, the customer thread is unblocked and it waits for the semaphore `barista_sem` to be greater than 0. When it becomes greater than 0, it marks itself as waiting to be picked up by barista.
- Then customer thread sem_post `orders_sem` and waits for either its order to be processed or for its tolerance time to be over. If its order going to be processed, it waits for the semaphore `order_complete` of its order number to be 1. If its tolerance time is going to be over, it waits for that time to be over and then checks if its order is processed or not. If its order is processed, it takes the order and leaves the cafe. If its order is not processed, it leaves the cafe without taking the order.
- Barista thread is running in infinite while(1) loop. To break this loop when all orders are served and no customer is waiting, I have used a variable `served` which is initialized with 0. When a customer order is processed, it is incremented by 1. I have also used variable `orders` which is initialized with the number of orders. When a customer leaves the cafe without taking the order, it is decremented by 1. When `served` becomes equal to `orders`, the barista thread breaks the loop and exits.
- Same as Customer thread, To implement the concept of minimum barista first I have used `barista_status` hash array and semaphore array `wakeup`.
- After barista get chance, it waits for the semaphore `orders_sem` to be greater than 0. When it becomes greater than 0, it locks the mutex `orderslock` and checks for the minimum order number. If the order is not processed, it unlocks the mutex `orderslock` and starts processing the order. After order is processed, it locks the mutex `orderslock` and sets the semaphore `order_complete` of that order number to 1. Then it unlocks the mutex `orderslock` and increments the variable `served` by 1. It also increments the semaphore `barista_sem` by 1.

### How to run

- To run the cafe sim, run the following command in the terminal:
```
gcc 1.c -lpthread
./a.out
```

### Assumptions
- If barista becomes free at time t, then it can take order at time t+1.

## Ice Cream Parlor Sim

### Implementation

- For Implementation of the ice cream parlor sim, I have used the concept of multithreading, mutex locks and semaphores.
- First, I have created a thread for each customer and each machine.
- I have created a mutex lock for `order processing(orderslock)` and `printing details(printlock)`.
- I have used semaphore `machine_order` which is initialized with 0. When a machine thread starts, it waits for the semaphore `machine_order` to be greater than 0. When it becomes greater than 0, it will process the order.
- When a customer thread starts, it will check if there is space available or not. If there is no space then customer will leave.
- Then customer check for ingredients. If all ingredients were not available, then we will mark `ingrediants_shortage` as 1.
- Customer will give order. then waits for either ingredients shortage alert or order to be processed or parlor to be closed.
- Now when machine thread wakes up, it will wait for the semaphore `machine_order` to be greater than 0. When it becomes greater than 0, it will lock the mutex `orderslock`. Then it will minimum available customer and minimum order number. Then it will check if that customer is waiting for ingredients shortage alert or not. If it is waiting for ingredients shortage alert, then it will give alert and find next minimum order number. If it is not waiting for ingredients shortage alert, then it will check if it has sufficient time to process the order or not. If it does not have sufficient time, then it will find next minimum order number. If it has sufficient time, then it will again check if there are sufficient ingredients or not. If there are sufficient ingredients, then it will process the order and decrement the ingredients. If there are not sufficient ingredients, then it will find next minimum order number. After processing the order, it will unlock the mutex `orderslock` and give the order to the customer via semaphore `customer_order_complete` of that customer number.

### How to run

- To run the ice cream parlor sim, run the following command in the terminal:
```
gcc 2.c -lpthread
./a.out
```

### Assumptions
- When a machine picks an order of customer, it will check if it has sufficient ingredients or not for all the orders of that customer. If it has sufficient ingredients for all the orders of that customer, then only it will process that particular order. If it does not have sufficient ingredients for all the orders of that customer, then it will send ingredients shortage alert to that customer and find next minimum order number. This is done to avoid wastage of ingredients..

### Further Refinement

- **Minimizing Incomplete Orders**
    - To Minimize the Incomplete Orders, we have to check both insufficient ingredients and insufficient time. If we check only insufficient ingredients, then it will lead to wastage of time. If we check only insufficient time, then it will lead to wastage of ingredients. So, we have to check both. To check that we can check if we have sufficient ingredients for all the orders of that customer or not. If we have sufficient ingredients for all the orders of that customer, then only we will process that particular order. If we do not have sufficient ingredients for all the orders of that customer, then we will reject that order. This will minimize the incomplete orders. Now if we have sufficient ingredients for all the orders of that customer, then we will check if we have sufficient time to process that order or not. We will check if all orders can be processed before closing time or not. If all orders can be processed before closing time, then we will process that order. If all orders cannot be processed before closing time, then we will immediately reject that order. This will minimize the incomplete orders.

- **Ingredient Replenishment**
    - If we can Replensih ingredients,then it means that we have infinite ingredients. So, To Minimize Incomplete Orders we can check only insufficient time. If we have sufficient time to process that order, then we will process that order. To implement this, we can check if all orders can be processed before closing time or not. If all orders can be processed before closing time, then we will process that order. If all orders cannot be processed before closing time, then we will immediately reject that order. This will minimize the incomplete orders.

- **Unserviced Orders**
    - To minimize the number of unserviced orders or customers having to wait until the parlor closes to get their order, we can check if all orders can be processed before closing time or not. If all orders can be processed before closing time, then we will reserve that time for that customer so no other customer can take that time. If all orders cannot be processed before closing time, then we will immediately reject that order. This saves the time of the customer and also minimizes the number of unserviced orders and also reputation of the parlor.