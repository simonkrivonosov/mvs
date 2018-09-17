#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "graph.h"
#include "random_generator.h"
#include "thread_pool.h"

#define PACK_SIZE 655360

#define MUTATION_RATE 0.1
#define SELECTION_RATE 0.75

graph_t *graph;

typedef struct{
  int *route;
  int fitness;
  int cycle_counter;
} route_t;

route_t solve_tsp_genetic(int population_size, int stopping_time,
                          thread_pool_t *pool, random_data_t *rnd,
                          int verbose);

void generate_route(void *arg, random_data_t *rnd);

void calculate_fitness(void *arg, random_data_t *rnd);

/* exchange 2 vertices (select 1-neighbour) */
void mutate(void *arg, random_data_t *rnd);

/* select random_ part of first parent and complete in order of second */
typedef struct {
  route_t *parent1, *parent2;
  route_t *child;
  int *buffer;
} crossover_args_t;
void crossover(void *arg, random_data_t *rnd);

int update_best(route_t *best, route_t *candidates, int candidate_count);

void dump_array(FILE *f, const int *array, int n);

int vertex_weight(route_t *r, int i);

int main(int argc, char **argv) {

  int thread_count;
  int population_size;
  int stopping_time;
  int verbose;
  thread_pool_t pool;
  random_data_t rnd;
  route_t solution;

  struct timespec begin_time, end_time;
  double elapsed_time;
  FILE *stats;

  assert(argc == 6 );
  thread_count = atoi(argv[1]);
  population_size = atoi(argv[2]);
  stopping_time = atoi(argv[3]);
  int graph_size;
  if (!strcmp(argv[4], "--generate")) {
    srand(time(NULL));
    graph = graph_generate(atoi(argv[5]), 100);
    graph_size = atoi(argv[5]);
  } else {
    graph = graph_read_file(argv[5]);
    graph_size = graph->n;
  }

  random_init(time(NULL) + 1, PACK_SIZE, thread_count + 2);
  random_data_init(&rnd);
  thread_pool_init(&pool, thread_count, population_size);

  clock_gettime(CLOCK_MONOTONIC, &begin_time);
  solution = solve_tsp_genetic(population_size, stopping_time, &pool, &rnd, verbose);
  clock_gettime(CLOCK_MONOTONIC, &end_time);

  elapsed_time = end_time.tv_sec - begin_time.tv_sec
               + (end_time.tv_nsec - begin_time.tv_nsec) * 1e-9;

  stats = fopen("stats.txt", "w");
  fprintf(stats, "%d %d %d %d %d %fs %d\n", thread_count, population_size,
          stopping_time, graph_size, solution.cycle_counter, elapsed_time, solution.fitness);
  dump_array(stats, solution.route, graph->n);
  fclose(stats);

  free(solution.route);

  thread_pool_destroy(&pool);
  random_data_destroy(&rnd);
  random_destroy();
  graph_destroy(graph);

  return EXIT_SUCCESS;
}

route_t solve_tsp_genetic(int population_size, int stopping_time,
                          thread_pool_t *pool, random_data_t *rnd,
                          int verbose) {
  route_t *population = malloc(population_size * sizeof(route_t));
  route_t best = { malloc(graph->n * sizeof(int)), INT_MAX, 0};
  int improvement_time = 0;
  int i;
  int selection_size = (int) (population_size * SELECTION_RATE);

  crossover_args_t *crossover_args = malloc(selection_size * sizeof(crossover_args_t));
  for (i = 0; i < selection_size; ++i) {
    crossover_args[i].buffer = malloc(graph->n * sizeof(int));
  }

  for (i = 0; i < population_size; ++i) {
    task_t generate_task = { generate_route, population + i };
    population[i].route = malloc(graph->n * sizeof(int));
    thread_pool_submit(pool, generate_task);
  }
  thread_pool_wait_all(pool);
  update_best(&best, population, population_size);
  int cycle_counter = 0;
  while (improvement_time++ < stopping_time) {
    int selected;
    cycle_counter++;

    for (selected = 0; selected < selection_size; ++selected) {
      int u = random_(rnd) % (population_size - selected);
      int v = random_(rnd) % (population_size - selected);
      int worse = population[u].fitness < population[v].fitness ? v : u;

      route_t tmp = population[population_size - selected - 1];
      population[population_size - selected - 1] = population[worse];
      population[worse] = tmp;
    }

    for (i = 0; i < selected; ++i) {
      int u = random_(rnd) % (population_size - selected);
      int v = random_(rnd) % (population_size - selected);
      task_t crossover_task = { crossover, crossover_args + i };

      crossover_args[i].parent1 = population + u;
      crossover_args[i].parent2 = population + v;
      crossover_args[i].child = population + population_size - i - 1;

      thread_pool_submit(pool, crossover_task);
    }
    thread_pool_wait_all(pool);

    for (i = 0; i < population_size; ++i) {
      if ((double) random_(rnd) / RAND_MAX < MUTATION_RATE) {
        task_t mutation_task = { mutate, population + i };
        thread_pool_submit(pool, mutation_task);
      }
    }
    thread_pool_wait_all(pool);

    if (update_best(&best, population, population_size)) {
      improvement_time = 0;
    }

    if (verbose) {
      int min = INT_MAX, max = 0;
      double mean = 0;

      for (i = 0; i < population_size; ++i) {
        min = min > population[i].fitness ? population[i].fitness : min;
        max = max < population[i].fitness ? population[i].fitness : max;
        mean += (double) population[i].fitness / population_size;
      }
    }
  }
  best.cycle_counter = cycle_counter;

  for (i = 0; i < population_size; ++i) {
    if (i < selection_size) {
      free(crossover_args[i].buffer);
    }

    free(population[i].route);
  }
  free(crossover_args);
  free(population);

  return best;
}

void generate_route(void *arg, random_data_t *rnd) {
  route_t *r = (route_t *) arg;
  int i;

  for (i = 0; i < graph->n; ++i) {
    r->route[i] = i;
  }

  for (i = 0; i < graph->n; ++i) {
    int tmp = r->route[i];
    int j = i + random_(rnd) % (graph->n - i);
    r->route[i] = r->route[j];
    r->route[j] = tmp;
  }

  calculate_fitness(r, rnd);
}

void calculate_fitness(void *arg, random_data_t *rnd) {
  int i;
  route_t *r = (route_t *) arg;

  r->fitness = graph_weight(graph, r->route[graph->n - 1], r->route[0]);
  for (i = 1; i < graph->n; ++i) {
    r->fitness += graph_weight(graph, r->route[i - 1], r->route[i]);
  }
}

void mutate(void *arg, random_data_t *rnd) {
  route_t *r = (route_t *) arg;
  int i = random_(rnd) % graph->n;
  int j = random_(rnd) % graph->n;

  r->fitness -= vertex_weight(r, i);
  r->fitness -= vertex_weight(r, j);

  int tmp = r->route[i];
  r->route[i] = r->route[j];
  r->route[j] = tmp;

  r->fitness += vertex_weight(r, i);
  r->fitness += vertex_weight(r, j);
}

void crossover(void *arg, random_data_t *rnd) {
  route_t *p1 = ((crossover_args_t *) arg)->parent1;
  route_t *p2 = ((crossover_args_t *) arg)->parent2;
  route_t *child = ((crossover_args_t *) arg)->child;
  int *used = ((crossover_args_t *) arg)->buffer;

  int start_1 = random_(rnd) % graph->n;
  int len_1 = random_(rnd) % graph->n;
  int i, j;

  memset(used, 0, graph->n * sizeof(int));
  for (i = 0; i < len_1; ++i) {
    child->route[i] = p1->route[(start_1 + i) % graph->n];
    used[child->route[i]] = 1;
  }

  j = 0;
  while (i < graph->n) {
    while (used[p2->route[j]])
      ++j;
    child->route[i++] = p2->route[j++];
  }

  calculate_fitness(child, rnd);
}

int update_best(route_t *best, route_t *candidates, int candidate_count) {
  int i;
  int new_best = -1;

  for (i = 0; i < candidate_count; ++i) {
    if (candidates[i].fitness < best->fitness) {
      best->fitness = candidates[i].fitness;
      new_best = i;
    }
  }

  if (new_best >= 0) {
    memcpy(best->route, candidates[new_best].route, graph->n * sizeof(int));
    new_best = -1;
    return 1;
  } else {
    return 0;
  }
}

int vertex_weight(route_t *r, int i) {
  return graph_weight(graph, r->route[i], r->route[(i + 1) % graph->n])
       + graph_weight(graph, r->route[(i ? i : graph->n) - 1], r->route[i]);
}

void dump_array(FILE *f, const int *array, int n) {
  int i;

  if (n <= 0)
    return;

  fprintf(f, "%d", array[0]);
  for (i = 1; i < n; ++i)
    fprintf(f, " %d", array[i]);
  fputc('\n', f);
}

