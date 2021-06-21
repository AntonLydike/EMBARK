#ifndef H_MUTEX
#define H_MUTEX

// mutex operations (modifies data, no checks)
int  mutex_create();
void mutex_lock(int mutex_id);
void mutex_unlock(int mutex_id);
void mutex_destroy(int mutex_id);
// mutex helpers
int  mutex_is_locked(int mutex_id);


#endif