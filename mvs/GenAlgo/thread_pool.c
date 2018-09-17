#include "thread_pool.h"

#include <stdlib.h>

#include <pthread.h>

void * thread_pool_worker(void *arg) {
  thread_pool_t *pool = (thread_pool_t *) arg;
  random_data_t rnd;

  random_data_init(&rnd);

  pthread_mutex_lock(&pool->lock);
  for (;;) {
    if (++pool->idle_count == pool->thread_count) {
      pthread_cond_signal(&pool->done_cond);
    }

    while (!(pool->destroyed || pool->task_count)) {
      pthread_cond_wait(&pool->task_cond, &pool->lock);
    }

    if (pool->task_count) {
      task_t task = pool->tasks[--pool->task_count];

      --pool->idle_count;
      pthread_mutex_unlock(&pool->lock);
      task.func(task.arg, &rnd);
      pthread_mutex_lock(&pool->lock);
    } else {
      break;
    }
  }

  pthread_mutex_unlock(&pool->lock);

  random_data_destroy(&rnd);

  return NULL;
}

void thread_pool_init(thread_pool_t *pool, int thread_count, int capacity) {
  int i;

  pool->destroyed = 0;
  pool->thread_count = thread_count;
  pool->idle_count = 0;

  pool->threads = (pthread_t *) malloc(thread_count * sizeof(pthread_t));
  pthread_cond_init(&pool->task_cond, NULL);
  pthread_cond_init(&pool->done_cond, NULL);

  pool->tasks = (task_t *) malloc(capacity * sizeof(task_t));
  pool->task_count = 0;
  pool->task_capacity = capacity;

  pthread_mutex_init(&pool->lock, NULL);

  for (i = 0; i < thread_count; ++i) {
    pthread_create(&pool->threads[i], NULL, thread_pool_worker, pool);
  }
}

int thread_pool_submit(thread_pool_t *pool, task_t task) {
  pthread_mutex_lock(&pool->lock);
  if (pool->destroyed) {
    return 0;
  }

  if (pool->task_count == pool->task_capacity) {
    pool->task_capacity *= 2;
    pool->tasks = (task_t *) realloc(pool->tasks,
                                     pool->task_capacity * sizeof(task_t));
  }

  pool->tasks[pool->task_count++] = task;
  pthread_mutex_unlock(&pool->lock);

  pthread_cond_signal(&pool->task_cond);

  return 0;
}

void thread_pool_wait_all(thread_pool_t *pool) {
  pthread_mutex_lock(&pool->lock);
  while (pool->idle_count < pool->thread_count || pool->task_count) {
    pthread_cond_wait(&pool->done_cond, &pool->lock);
  }
  pthread_mutex_unlock(&pool->lock);
}

void thread_pool_destroy(thread_pool_t *pool) {
  int i;

  pthread_mutex_lock(&pool->lock);
  pool->destroyed = 1;
  pthread_mutex_unlock(&pool->lock);

  pthread_cond_broadcast(&pool->task_cond);
  for (i = 0; i < pool->thread_count; ++i) {
    pthread_join(pool->threads[i], NULL);
  }
  free(pool->threads);
  pthread_cond_destroy(&pool->task_cond);
  pthread_cond_destroy(&pool->done_cond);

  free(pool->tasks);

  pthread_mutex_destroy(&pool->lock);
}

