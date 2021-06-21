#include "mutex.h"
#include "../kernel.h"

// mutex lock structure:
// this is a dict describing which process instantiated the lock
// it maps mutex_id -> pid (or zero for unused locks)
int locks[MUTEX_COUNT]; 
int locks_bitfield[MUTEX_COUNT / XLEN]; // each bit representing if the lock is
                                        // engaged


int  mutex_is_locked(int mutex_id)
{
    int offset = mutex_id % XLEN;

    return locks[mutex_id / XLEN] & (1 << offset);
}

int  mutex_create()
{
    return 0;
}

void mutex_lock(int mutex_id)
{

}

void mutex_unlock(int mutex_id)
{

}

void mutex_destroy(int mutex_id)
{

}
