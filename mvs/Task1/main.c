//
//  main.c
//  random_walk
//
//  Created by simon on 09.10.2017.
//  Copyright © 2017 simon. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>
#include <assert.h>


typedef struct parameters {
    int a;
    int b;
    int x;
    int N;
    double p;
    int P;
    int cmp_num;
} parameters_t;

typedef struct particle_results_t {
    int finishes_in_b;
    int time;
} particle_results_t;

int Step(parameters_t * parameters, unsigned int * seed) {
    if(parameters->cmp_num > rand_r(seed))
        return 1;
    else
        return -1;
}
particle_results_t RandomWalk(parameters_t * parameters, int i) {
    particle_results_t results = {
        .finishes_in_b = 0,
        .time = 0,
    };
    unsigned int seed = i;
    int step, current_pos;
    current_pos = parameters->x;
    while(1) {
        step = Step(parameters, &seed);
        results.time++;
        current_pos += step;
        if(current_pos == parameters->b) {
            results.finishes_in_b++;
            break;
        }
        if(current_pos == parameters->a)
            break;
    }
    return results;
}

int main(int argc, const char * argv[]) {
    assert(argc == 7);
    int a = atoi(argv[1]);
    int b = atoi(argv[2]);
    int x = atoi(argv[3]);
    int N = atoi(argv[4]);
    int P = atoi(argv[6]);
    double p = atof(argv[5]);
    assert(b > a);
    assert(x > a && x < b);
    omp_set_num_threads(P);
    parameters_t parameters = {
        .a = a,
        .b = b,
        .x = x,
        .p = p,
        .N = N,
        .P = P,
        .cmp_num = (int)(p *  RAND_MAX),
    };
    int all_finishes_in_b = 0;
    int all_time = 0;
    double start_time = omp_get_wtime();
    int seed;
    srand(time(NULL));
#pragma omp parallel for private(seed) reduction(+: all_time, all_finishes_in_b)
    for(int i = 0; i < parameters.N; ++i) {
        seed = rand();
        particle_results_t res = RandomWalk(&parameters, seed);
        all_time += res.time;
        all_finishes_in_b += res.finishes_in_b;
    }
    double end_time = omp_get_wtime();
    double b_probability = (double)all_finishes_in_b / parameters.N;
    double average_time = (double)all_time / parameters.N;
    FILE * file;
    file = fopen("stats.txt", "ab");
    if(file == NULL)
        perror("unable to open the file");
    fprintf(file,"b_probability = %f, existing_time = %f, program_time = %f, a = %d, b = %d, x = %d, N = %d, p = %f, P = %d\n", b_probability, average_time, end_time - start_time, a, b, x, N, p, P);
    fclose(file);
    return 0;
}

