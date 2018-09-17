#include "random_generator.h"

#include <stdlib.h>
#include <pthread.h>

static size_t random_pack_size;
static size_t random_max_pack_count;

static int **random_packs;
static int random_destroyed = 0;
static unsigned int random_state;
static size_t random_pack_count = 0;

static pthread_t random_thread;
static pthread_mutex_t random_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t random_cond_full = PTHREAD_COND_INITIALIZER;
static pthread_cond_t random_cond_empty = PTHREAD_COND_INITIALIZER;

void * random_generator(void *arg) {
  pthread_mutex_lock(&random_lock);
  for (;;) {
    int *pack;
    size_t i;

    while (!(random_destroyed
             || random_pack_count < random_max_pack_count)) {
      pthread_cond_wait(&random_cond_full, &random_lock);
    }

    if (random_destroyed)
      break;

    pthread_mutex_unlock(&random_lock);
    pack = (int *) malloc(random_pack_size * sizeof(int));
    for (i = 0; i < random_pack_size; ++i) {
      pack[i] = rand_r(&random_state);
    }
    pthread_mutex_lock(&random_lock);

    if (!random_pack_count) {
      pthread_cond_signal(&random_cond_empty);
    }
    random_packs[random_pack_count++] = pack;
  }
  pthread_mutex_unlock(&random_lock);

  return NULL;
}

void random_init(unsigned int seed, size_t pack_size, size_t max_pack_count) {
  random_pack_size = pack_size;
  random_max_pack_count = max_pack_count;
  random_state = seed;
  random_packs = (int **) malloc(random_max_pack_count * sizeof(int *));

  pthread_create(&random_thread, NULL, random_generator, NULL);
}

void random_destroy() {
  pthread_mutex_lock(&random_lock);
  random_destroyed = 1;
  while (random_pack_count) {
    free(random_packs[--random_pack_count]);
  }
  free(random_packs);
  pthread_mutex_unlock(&random_lock);

  pthread_cond_signal(&random_cond_full);
  pthread_join(random_thread, NULL);

  pthread_mutex_destroy(&random_lock);
  pthread_cond_destroy(&random_cond_full);
  pthread_cond_destroy(&random_cond_empty);
}

void random_data_init(random_data_t *data) {
  data->used = random_pack_size;
  data->pack = NULL;
}

void random_data_destroy(random_data_t *data) {
  free(data->pack);
}

int random_(random_data_t *data) {
  if (data->used == random_pack_size) {
    data->used = 0;
    free(data->pack);

    pthread_mutex_lock(&random_lock);
    while (!random_pack_count) {
      pthread_cond_wait(&random_cond_empty, &random_lock);
    }
    data->pack = random_packs[--random_pack_count];
    pthread_mutex_unlock(&random_lock);

    pthread_cond_signal(&random_cond_full);
  }

  return data->pack[data->used++];
}

