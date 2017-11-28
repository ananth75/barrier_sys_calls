//
//  main.c
//  
//
//  Created by Ananth Mudumba on 11/26/17.
//
//

#include <stdio.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <sys/syscall.h>

#define T_COUNT1 2
#define T_COUNT2 0

signed int timeout;
int interval_between_iter;
int iterations;

void *thread1_function(void *args)
{
    int ret;
    int i;
    int *b_id = (int *)args;
    for (i = 0; i < iterations; i++)
    {
        
        /* Reference: https://stackoverflow.com/questions/13219501/accessing-a-system-call-directly-from-user-program */
        
        /* DBGG: Check this */
        //printf(" Thread with id %d entering %dth round of synchronization in the child process with  pid %d \n", syscall(SYS_gettid), (i+1),  syscall(SYS_getpid));
        
        ret = syscall(361, *b_id);
        printf("USPace args int %d and ret %d\n ", *b_id, ret);
        if(ret < 0)
        {
            printf("\nBarrier wait failed with error %s\n", strerror(errno));
        }
        //printf(" Thread with id %d exiting %dth round of synchronization in the child process with  pid %d \n", syscall(SYS_gettid), (i+1),  syscall(SYS_getpid));
        
        /* DBGG TODO Randomize this value */
        //usleep(interval_between_iter);
        sleep(3);
    }
    pthread_exit(0);
}

void *thread2_function(void *args)
{
    int ret;
    int i;
    int *b_id = (int *)args;
    for (i = 0; i < iterations; i++)
    {
        
        /* Reference: https://stackoverflow.com/questions/13219501/accessing-a-system-call-directly-from-user-program */
        
        /* DBGG: Check this */
        //printf(" Thread with id %d entering %dth round of synchronization in the child process with  pid %d \n", syscall(SYS_gettid), (i+1),  syscall(SYS_getpid));
        
        
        ret = 	syscall(361, *b_id);
        printf("USPace args int %d and ret %d\n ", *b_id, ret);
        if(ret < 0)
        {
            printf("\nBarrier wait failed with error %s\n", strerror(errno));
        }
        //printf(" Thread with id %d exiting %dth round of synchronization in the child process with  pid %d \n", syscall(SYS_gettid), (i+1),  syscall(SYS_getpid));
        
        /* DBGG TODO Randomize this value */
        //usleep(interval_between_iter);
        sleep(3);
    }
    pthread_exit(0);
}
void Child()
{
    
    pid_t pidc;
    pthread_t id1[T_COUNT1], id2[T_COUNT2];
    unsigned int barrier_id1, barrier_id2;
    int ret, thread_count, timeout;
    int i;
    
    
    pidc = getpid();
    printf("Child process with pid %d entered \n", pidc );
    
    
    /* Initiliaze the barrier with 5 threads first */
    thread_count = T_COUNT1;
    
    ret = syscall(360, thread_count, &barrier_id1, timeout);
    printf("USPace Child ret %d and barrier id %d \n ", ret, barrier_id1);
    if( ret == -1);
    {
        printf("\nBarrier initialization failed ");
    }
    
    //printf("\nBarrier id %d created ", barrier_id1);
    
    /* Create 5 threads and pass them the created the barrier id that they could wait on */
    for(i = 0; i < T_COUNT1; i++)
    {
        pthread_create(&id1[i], NULL, thread1_function, (void *) &barrier_id1);
    }
    
    /* Initiliaze the barrier with 20 threads now */
    thread_count = T_COUNT2;
    
    ret = syscall(360, thread_count, &barrier_id2, timeout);
    printf("USpace Child ret %d and barrier id %d\n", ret, barrier_id2); 
    if(ret == -1)
    {
        printf("\nBarrier initialization failed ");
    }
    
    //printf("Barrier id %d created ", barrier_id2);
    
    /* Create 5 threads and pass them the created the barrier id that they could wait on */
    for(i = 0; i < T_COUNT2; i++)
    {
        /* DBGG Check */
        pthread_create(&id2[i], NULL, thread2_function, (void *) &barrier_id2);
    }
    
    for(i = 0; i < T_COUNT1; i++)
    {
        pthread_join(id1[i], NULL);
    }
    
    for(i = 0; i < T_COUNT2; i++)
    {
        pthread_join(id2[i], NULL);
    }
    
    
    sleep(1);
    
    if ((ret = syscall(362, barrier_id1)) < 0)
    {
        printf("\nError : barrier id %d not destroyed \n",barrier_id1);
    }
    
sleep(1);
    if ((ret = syscall(362, barrier_id2)) < 0)
    {
        printf("\nError : barrier id %d not destroyed \n",barrier_id2);
    }
}


int main( int argc, char *argv[], char *env[] )
{
    
    if(argc != 2)
    {
        printf("\nEnter the timeout between the consecutive iterations in microseconds ");
        exit(1);
    }
    
    interval_between_iter = atoi(argv[1]);
    
    int status = 0;
    timeout = 500;
    iterations = 2;
    
    /* Create PID(s) for child processes */
    pid_t child_pid_one, child_pid_two;
    
    
    child_pid_one = fork();
    
    if(child_pid_one < 0)
    {
        printf("\nChild Process 1 creation failed");
        exit(1);
    }
    
    /* Child one creation successful */
    else if(child_pid_one == 0)
    {
        Child();
    }

    /* Parent process */
    //else if(child_pid_one > 0)
    else
    {
        child_pid_two = fork();
        if(child_pid_two < 0)
        {
            printf("\nChild Process 2 creation failed");
            exit(1);
        }
        
        /* Child one creation successful */
        else if(child_pid_two == 0)
        {
            Child();
        }
        //DBGG
        //else if(child_pid_two > 0)
        else
        {
            /* Now that two child processes have been created successfully, we wait for the children to finish */
            wait(NULL);
        }
        wait(NULL);
    }
    return 0;
}
