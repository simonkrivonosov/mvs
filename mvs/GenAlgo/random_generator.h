#ifndef RANDOM_GENERATOR_H_
#define RANDOM_GENERATOR_H_

#include <stddef.h>

void random_init(unsigned int seed, size_t pack_size, size_t max_pack_count);
void random_destroy();

typedef struct {
  int *pack;
  size_t used;
} random_data_t;

void random_data_init(random_data_t *data);
void random_data_destroy(random_data_t *data);

int random_(random_data_t *data);
#endif 

