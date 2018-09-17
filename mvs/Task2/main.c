#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <omp.h>
#include <assert.h>

int m;

int BinarySearch(int* array, int key, int left, int right)
{
    if(right - left < 2) {
        if(array[left] > key)
            return left;
        else if( array[right] < key)
            return right + 1;
        else
            return right;
    }
    else {
        int mid = left + (right - left) / 2;

        if (array[mid] == key)
            return mid;

        else if (array[mid] > key)
            return BinarySearch(array, key, left, mid);
        else
            return BinarySearch(array, key, mid + 1, right);
    }
}

int CMP(const void * a, const void * b) {
    return ( *(int*)a - *(int*)b );
}

void Merge(int* arr, int l1, int r1, int l2, int r2, int* result) {
    int i = 0;
    int j = 0;

    while((i + l1 < r1 + 1) && (j + l2 < r2 + 1)) {
        if(arr[l1 + i] < arr[l2 + j]) {
            result[i + j] = arr[l1 + i];
            i++;
        } else {
            result[i + j] = arr[l2 + j];
            j++;
        }
    }
    while(i + l1 < r1 + 1) {
        result[i + j] = arr[l1 + i];
        i++;
    }
    while(j + l2 < r2 + 1) {
        result[i + j] = arr[l2 + j];
        j++;
    }
}

void ParallelMerge(int* arr, int l1, int r1, int l2, int r2, int* result) {
    int q1 = (l1 + r1) / 2;
    int q2 = BinarySearch(arr, arr[q1], l2, r2);
    int q3 = (q1 - l1) + (q2 - l2);
    result[q3] = arr[q1];
    #pragma omp parallel
    {
        #pragma omp sections 
        {
            #pragma omp section
            {
                Merge(arr, l1, q1 - 1, l2, q2 - 1, result);
            }
            #pragma omp section
            {
                Merge(arr, q1 + 1, r1, q2, r2, &result[q3 + 1]);
            }
        }
    }
    return;
}


void ParallelMergeSort(int* arr, int l, int r, int* result) {
    if (r - l <= m) {
        qsort(&arr[l], r - l + 1, sizeof(int), CMP);
        memcpy(result, &arr[l], (r - l + 1) * sizeof(int));
        return;
    }
    int* temp =  (int *) malloc(sizeof(int) * (r - l + 1));
    int q = (l + r) / 2;
    #pragma omp parallel
    {
    	#pragma omp single nowait
    	{
	    #pragma omp task
	    ParallelMergeSort(arr, l, q, temp);

	    #pragma omp task
	    ParallelMergeSort(arr, q + 1, r, &temp[q - l + 1]);
    	}
    	#pragma omp taskwait
    }
    if ((q - l + 1) == 0 || (r - q) == 0)
        return;
    else
    	ParallelMerge(temp, 0, q - l, q - l + 1, r - l, result);
    free(temp);
    return;
}

int main(int argc, char** argv) {
    int n;
    int P;

    assert(argc == 4);

    n = atoi(argv[1]);
    m = atoi(argv[2]);
    P = atoi(argv[3]);
    omp_set_num_threads(P);


    int* array_for_quick_sort = (int *) malloc(sizeof(int) * n);
    if (array_for_quick_sort == NULL) {
        printf("memory allocation error\n");
        exit(1);
    }

    int* array_for_merge_sort = (int *) malloc(sizeof(int) * n);
    if (array_for_merge_sort == NULL) {
        printf("memory allocation error\n");
        exit(1);
    }

    int* result_merge_sort = (int *) malloc(sizeof(int) * n);
    if (result_merge_sort == NULL) {
        printf("memory allocation error\n");
        exit(1);
    }
    int* result_quick_sort = (int *) malloc(sizeof(int) * n);
    if (result_quick_sort == NULL) {
        printf("memory allocation error\n");
        exit(1);
    }
    srand(time(NULL));
    int t ;
    for (int i = 0; i < n ; i++) {
        t = rand() % n;
        array_for_merge_sort[i] = t;
        array_for_quick_sort[i] = t;
    }
    printf("\n");
    double start_time = omp_get_wtime();
    ParallelMergeSort(array_for_merge_sort, 0, n - 1, result_merge_sort);
    double end_time = omp_get_wtime();
    double start_time2 = omp_get_wtime();
    qsort(array_for_quick_sort, n, sizeof(int), CMP);
    double end_time2 = omp_get_wtime();
    FILE * file1;
    FILE * file2;
    file1 = fopen("stats.txt", "ab");
    if(file1 == NULL)
        perror("unable to open the file");
    fprintf(file1, "%f %d %d %d", end_time - start_time, n, m, P);
    file2 = fopen("data.txt", "ab");
    if(file2 == NULL)
        perror("unable to open the file");
    for(int i = 0; i < n; i++)
    {
        fprintf(file2, "%d ", array_for_merge_sort[i]);
    }
    fprintf(file2,"\n");
    for(int i = 0; i < n; i++)
    {
        fprintf(file2, "%d ", result_merge_sort[i]);
    }
    fclose(file1);
    fclose(file2);

    printf("%f ",end_time - start_time);
    printf("%f",end_time2 - start_time2);
    if(((end_time - start_time) - (end_time2 - start_time2)) < 0)
        printf("\nit is good\n");
    else
        printf("\nit is bad\n");
    free(array_for_merge_sort);
    free(array_for_quick_sort);
    free(result_merge_sort);
    free(result_quick_sort);
    return 0;
}

