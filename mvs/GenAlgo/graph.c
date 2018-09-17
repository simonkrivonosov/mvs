#include "graph.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

graph_t * graph_generate(const int n, const int w) {
  int i, j;
  int *weights = malloc(n * n * sizeof(int));
  assert(weights);

  graph_t *g = calloc(1, sizeof(graph_t));
  assert(g);
  g->n = n;
  g->weights = weights;

  for (i = 0; i < n; i++) {
    weights[i * n + i] = -1;
    for (j = i + 1; j < n; j++) {
      int weight = rand() % w + 1;
      weights[i * n + j] = weight;
      weights[j * n + i] = weight;
    }
  }

  return g;
}

inline int graph_weight(const graph_t *g, const int a, const int b) {
  if (a > g->n || b > g->n) {
    return -1;
  } else {
    return g->weights[a * g->n + b];
  }
}

graph_t *graph_read(FILE *f) {
  int n;
  int *weights;
  graph_t *g;
  int i, j;

  assert(fscanf(f, "%d", &n));
  weights = (int *) malloc(n * n * sizeof(int));
  assert(weights);

  g = (graph_t *) calloc(1, sizeof(graph_t));
  assert(g);
  g->n = n;
  g->weights = weights;
  for (i = 0; i < n; i++) {
    for (j = 0; j < n; j++) {
      assert(fscanf(f, "%d", weights + i * n + j));
    }
  }

  /* check graph for consistency */
  for (i = 0; i < n; i++) {
    for (j = 0; j < n; j++) {
      assert(weights[i * n + j] == weights[j * n + i]);
      if (i == j) {
        assert(weights[i * n + i] == -1);
      }
    }
  }

  return g;
}


graph_t * graph_read_file(const char *filename) {
  FILE *f = fopen(filename, "r");
  graph_t *g;
  assert(f);
  g = graph_read(f);
  fclose(f);
  return g;
}


void graph_dump(const graph_t *g, FILE *f) {
  int i, j;
  assert(fprintf(f, "%d\n", g->n) > 0);
  for (i = 0; i < g->n; i++) {
    for (j = 0; j < g->n; j++) {
      assert(fprintf(f, "%d ", graph_weight(g, i, j)));
    }
    assert(fprintf(f, "\n"));
  }
}

void graph_dump_file(const graph_t *g, const char *filename) {
  FILE *f = fopen(filename, "w");
  assert(f);
  graph_dump(g, f);
  fclose(f);
}

void graph_destroy(graph_t *g) {
  free(g->weights);
  free(g);
}

