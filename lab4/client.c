/**
 * Example client program that uses thread pool.
 */

#include <stdio.h>
#include <unistd.h>
#include "threadpool.h"

struct data
{
    int a;
    int b;
};

void add(void *param)
{
    struct data *temp;
    temp = (struct data*)param;

    printf("I add two values %d and %d result = %d\n",temp->a, temp->b, temp->a + temp->b);
}

int main(void)
{
    // create some work to do
    struct data work1 = {5, 10};
    struct data work2 = {1, 2};
    struct data work3 = {3, 4};
    struct data work4 = {10, 20};
    struct data work5 = {7, 8};
    struct data work6 = {2, 3};
    struct data work7 = {4, 5};
    struct data work8 = {6, 7};
    struct data work9 = {8, 9};
    struct data work10 = {9, 1};

    // initialize the thread pool
    pool_init(5);

    // submit the work to the queue
    pool_submit(&add, &work1);
    pool_submit(&add, &work2);
    pool_submit(&add, &work3);
    pool_submit(&add, &work4);
    pool_submit(&add, &work5);
    pool_submit(&add, &work6);
    pool_submit(&add, &work7);
    pool_submit(&add, &work8);
    pool_submit(&add, &work9);
    pool_submit(&add, &work10);


    // may be helpful 
    //sleep(3);

    pool_shutdown();

    return 0;
}
