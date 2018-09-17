#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_

#include <pthread.h>

#include "random_generator.h"

typedef struct {
  void (* func) (void *, random_data_t *);
  void *arg;
} task_t;

typedef struct {
  int destroyed;
  int thread_count;
  int idle_count;

  pthread_t *threads;
  pthread_cond_t task_cond;
  pthread_cond_t done_cond;

  task_t *tasks;
  int task_count;
  int task_capacity;

  pthread_mutex_t lock;
} thread_pool_t;

void thread_pool_init(thread_pool_t *pool, int thread_count, int capacity);

int thread_pool_submit(thread_pool_t *pool, task_t task);

void thread_pool_wait_all(thread_pool_t *pool);

void thread_pool_destroy(thread_pool_t *pool);

#endif /* THREAD_POOL_H_ */

