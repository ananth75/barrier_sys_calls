//
//  barrier_sys.c
//  
//
//  Created by Ananth Mudumba on 11/22/17.
//
//

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/string.h>

typedef struct
{
    int m_thread_count;
    int m_thread_current_count;
    int m_barrier_id;
    signed int timeout;
    pid_t m_barrier_pid;
    spinlock_t m_lock;
    struct list_head m_barrier;
    wait_queue_head_t m_entry_point;
    wait_queue_head_t m_auxiliary_entry_point;
}eosi_barrier_t, *p_eosi_barrier_t;


static LIST_HEAD(barrier_global_list);
bool barrier_dequeue_started = 0;
bool barrier_dequeue_done = 0;
bool barrier_wait = 0;

int waiting_threads_count = 0;

/* Auxiliary threads are those which try to sys_barrier_wait() on a barrier while a barrier is still in the process of releasing the previous threads */
int waiting_auxiliary_threads_count = 0;

/*barrier destroy system call*/
asmlinkage long sys_barrier_destroy(unsigned int barrier_id)
{
    
    struct list_head *next, *temp;
    bool is_barrier_found = 0;
    p_eosi_barrier_t ibarrier;
    
    /* Used this instead of list_for_each for safe deletion */
    list_for_each_safe(next, temp, &barrier_global_list)
    {
        
        /* Get the pointer to each of the elements in the list */
        ibarrier = list_entry(next, eosi_barrier_t, m_barrier);
        
        if(ibarrier->m_barrier_pid == task_tgid_vnr(current) && ibarrier->m_barrier_id == barrier_id)
        {
            is_barrier_found = 1;
            
            printk("Barried id  %d destroyed with tgid %d", ibarrier->m_barrier_id, ibarrier->m_barrier_pid);
            
            /* Delete the element from the list */
            list_del(next);
            
            /* Free the structure */
            kfree(ibarrier);
        }
    }
    
    
    if(!is_barrier_found)
    {
        printk("Invalid ID");
        return -EINVAL;
    }
    
    return 0;
    
}


asmlinkage long sys_barrier_wait(unsigned int barrier_id)
{
    p_eosi_barrier_t ibarrier_iter;
    bool is_barrier_found = 0;
    
    /* Loop through the list and find the barrier which has been initiliazed */
    list_for_each_entry(ibarrier_iter, &barrier_global_list, m_barrier )
    {
        /*Check if the PID of the calling address matches with the one stored in the list and cross validate with the barrier ID as well */
        if(ibarrier_iter->barrier_id == barrier_id && ibarrier_iter->m_barrier_pid == task_tgid_vnr(current))
        {
            is_barrier_found = 1;
            break;
        }
    }
    
    if(is_barrier_found == 0)
    {
        /* Invalid argument barrier_id */
        return -EINVAL;
    }
    
    /* Lock the Mutex */
    spin_lock(&ibarrier_iter->m_Lock);
    
    if(ibarrier_iter->m_thread_current_count <= ibarrier_iter->m_thread_count)
    {
        
        ibarrier_iter->m_thread_current_count++;
        waiting_threads_count++;
        
        /* Unlock the Mutex */
    spin_unlock(&ibarrier_iter->m_lock);
        
        
        /* Check whether a barrier unlock is in progress */
        spi_lock(&ibarrier_iter->m_lock);
        /* This condition handles the threads whose barrier_wait sys call arrives before all the waiting threads in the barrier are being dequeued */
        if(barrier_dequeue_started)
        {
            waiting_auxiliary_threads_count++;
            spin_unlock(&ibarrier_iter->m_lock);
            wait_event_interruptible(ibarrier_iter->m_auxiliary_entry_point, barrier_dequeue_done);
        }
        else
        {
            spin_unlock(&ibarrier_iter->m_lock);
        }
        
        
        /* This waits for all the 5 threads to come into the barrier and then gets signalled to release the barrier */
        wait_event_interruptible(ibarrier_iter->m_entry_point, barrier_wait);
        
        spi_lock(&ibarrier_iter->m_lock);
        if(waiting_threads_count == 0)
        {
            if(waiting_auxiliary_threads_count > 0)
            {
                barrier_dequeue_done = 1;
            }
            barrier_wait = 1;
        }
        else
        {
            waiting_threads_count--;
        }
        spin_unlock(&ibarrier_iter->m_lock);
        wake_up_all(&ibarrier_iter->m_auxiliary_entry_point);
        
    }
    else
    {
        /* This unlocks the barrier and the waiting threads start dequeuing from the barrier */
        barrier_wait = 1;
        barrier_dequeue_started = 1;
        wake_up_all(&ibarrier_iter->m_entry_point);
        spin_unlock(&ibarrier_iter->m_lock);
    }
    return 0;
    
}

/* Barrier Init Sys call */
asmlinkage long barrier_init(unsigned int count, int *barrier_id, signed int timeout)
{
    p_eosi_barrier_t ibarrier, ibarrier_iter;
    int count = 1;
    
    if (!(pbarrier = kmalloc(sizeof(barrier_struct), GFP_KERNEL)))
    {
        printk("Kmalloc failed for barrier\n");
    }
    
    memset(pbarrier, 0, sizeof( barrier_struct));
    
    /* Allocates the number of waiting threads that are sent from init */
    ibarrier->m_thread_count = count;
    ibarrier->m_thread_current_count = 0;
    
    /* First Entry in the barrier list */
    if(list_empty(&barrier_global_list)
    {
        ibarrier->m_barrier_id = 1;
    }
     
    else
    {
        /* Check for already existing barriers and assign an id to the new one */
        list_for_each_entry(ibarrier_iter, &barrier_global_list, m_barrier)
        {
            if(ibarrier_iter->m_barrier_pid == task_tgid_vnr(current))
            {
                count++;
                ibarrier->m_barrier_id = ibarrier_iter->m_barrier_id + 1;
            
            }
        }
        
        /* When the new task that's try to init has a different pid/tgid than the currently running ones. Allocates id = 1 to the barrier as per assignment */
        if(count == 1)
        {
            ibarrier->m_barrier_id = 1;
        }
    }
       
    *barrier_id = ibarrier->m_barrier_id;
       
    /* Initiliaze other parameters */
    ibarrier->timeout = timeout;
    ibarrier->m_barrier_pid = task_tgid_vnr(current);
    spin_lock_init(&ibarrier->m_lock);
    init_waitqueue_head(&ibarrier->m_entry_point);
    
    /* Initiliaze per-barrier list head and add the reference to the global list of barriers */
    INIT_LIST_HEAD(&ibarrier->m_barrier);
    list_add(&ibarrier->m_barrier, &barrier_global_list );
       printk("The barrier has been initiliazed with tgid = %d and barrier id = %d \n", task_tgid_vnr(current), ibarrier->m_barrier_id);
       
    return 0;
       
}

