#pragma once

// process control
int spawn(void (*child)(void*), void* args);

void sleep(int length);

int join(int pid, int timeout);

int kill(int pid);

void __attribute__((noreturn)) exit(int code);

// locks
typedef int m_lock;

m_lock mutex_create();

int mutex_lock(m_lock lock, int timeout);

void mutex_unlock(m_lock lock);

void mutex_destroy(m_lock lock);
