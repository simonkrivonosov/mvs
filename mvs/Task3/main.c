#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <assert.h>

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

struct timeval tv1,tv2,dtv;

struct timezone tz;

void time_start() { gettimeofday(&tv1, &tz); }

double time_stop()
{
    gettimeofday(&tv2, &tz);
    dtv.tv_sec= tv2.tv_sec -tv1.tv_sec;
    dtv.tv_usec=tv2.tv_usec-tv1.tv_usec;
    if(dtv.tv_usec<0) { dtv.tv_sec--; dtv.tv_usec+=1000000; }
    return dtv.tv_sec + (float)dtv.tv_usec/1000000;

}
int n;
int P;
int m;

typedef struct sorting_data_t {
    int start_index;
    int amount_of_chunks_for_thread;
    int* chunks_array;

} sorting_data_t;

int CMP (const void * a, const void * b) {
    return ( *(int*)a - *(int*)b );
}



typedef struct pthread_data_t {
    int l1;
    int r1;
    int l2;
    int r2;
    int* array;
} pthread_data_t;

void* ThreadSorting(void* sorting_data_) {
    sorting_data_t* sorting_data = (sorting_data_t*) sorting_data_;
    int start_index = sorting_data->start_index;
    int amount_of_chunks_for_thread = sorting_data->amount_of_chunks_for_thread;
    int* chunks_array = sorting_data->chunks_array;

    int cur_index = start_index;
    for (int i = 0; i < amount_of_chunks_for_thread; i++) {
        qsort(&chunks_array[cur_index], m, sizeof(int), CMP);
        cur_index += m;
    }
    return NULL;
}

void* Merge(void* data_) {
    pthread_data_t* data = (pthread_data_t*) data_;
    int l1 = data->l1;
    int r1 = data->r1;
    int l2 = data->l2;
    int r2 = data->r2;
    int* array = data->array;

    int i = 0;
    int j = 0;
    int* temp1 = malloc((r1 - l1 + 1) * sizeof(int));
    int* temp2 = malloc((r2 - l2 + 1) * sizeof(int));


    memcpy(temp1, &array[l1], (r1 - l1 + 1) * sizeof(int));
    memcpy(temp2, &array[l2], (r2 - l2 + 1) * sizeof(int));

    while((i < (r1 - l1 + 1)) && (j < (r2 - l2 + 1))) {
        if(temp1[i] < temp2[j]) {
            array[l1 + i + j] = temp1[i];
            i++;
        } else {
            array[l1 + i + j] = temp2[j];
            j++;
        }
    }
    while(i < (r1 - l1 + 1)) {
        array[l1 + i + j] = temp1[i];
        i++;
    }
    while(j < (r2 - l2 + 1)) {
        array[l1 + i + j] = temp2[j];
        j++;
    }

    free(temp1);
    free(temp2);
    return NULL;
}

void InitialChunksSorting(int* array, int n) {
    pthread_t* threads = (pthread_t*) malloc(P * sizeof(pthread_t));
    sorting_data_t* thread_data = malloc(P * sizeof(sorting_data_t));

    int amount_of_chunks = n / m;
    int remain = n % m;
    int amount_of_chunks_for_thread = amount_of_chunks / P;

    int cur_index = 0;

    for(int i = 0; i < P; i++) {
        thread_data[i].start_index = cur_index;
        thread_data[i].amount_of_chunks_for_thread = amount_of_chunks_for_thread;
        thread_data[i].chunks_array = array;
        pthread_create(&(threads[i]), NULL, ThreadSorting, &thread_data[i]);
        cur_index += m * amount_of_chunks_for_thread;
    }
    for(int i = 0; i < P; i++) {
        pthread_join(threads[i], NULL);
    }
    if (remain != 0 || amount_of_chunks == 1)
        qsort(&array[cur_index], n - cur_index, sizeof(int), CMP);
    free(threads);
    free(thread_data);
}

void ParallelMergeSort(int* array, int n) {

    InitialChunksSorting(array, n);

    pthread_t* threads = malloc(P * sizeof(pthread_t));

    pthread_data_t* thread_data = malloc(P * sizeof(pthread_data_t));


    for (int cur_size = m; cur_size < n; cur_size *= 2) {
        int cur_index = 0;
        int num_threads = P;
        while(cur_index + cur_size + 1 < n) {
            for(int i = 0; i < P; i++) {
                int l1 = cur_index;
                int r1 = cur_index + cur_size - 1;
                if (r1 >= n) {
                    num_threads = i;
                    break;
                }
                int l2 = r1 + 1;
                int r2 = l2 + cur_size - 1;
                if (r2 > n - 1) {
                    r2 = n - 1;
                }
                cur_index = r2 + 1;

                thread_data[i].l1 = l1;
                thread_data[i].r1 = r1;
                thread_data[i].l2 = l2;
                thread_data[i].r2 = r2;
                thread_data[i].array = array;

                pthread_create(&(threads[i]), NULL, Merge, &thread_data[i]);
            }

            for(int i = 0; i < num_threads; i++) {
                pthread_join(threads[i], NULL);
            }
        }
    }

    free(threads);
    free(thread_data);
}

int main(int argc, char** argv) {


    assert(argc == 4);

    n = atoi(argv[1]);
    m = atoi(argv[2]);
    P = atoi(argv[3]);



    int* array_for_quick_sort = (int *) malloc(sizeof(int) * n);
    if (array_for_quick_sort == NULL) {
        printf("memory alarray error\n");
        exit(1);
    }

    int* array_for_merge_sort = (int *) malloc(sizeof(int) * n);
    if (array_for_merge_sort == NULL) {
        printf("memory alarray error\n");
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
    FILE * file1;
    FILE * file2;
    file2 = fopen("data.txt", "ab");
    if(file2 == NULL)
        perror("unable to open the file");
    for(int i = 0; i < n; i++)
    {
        fprintf(file2, "%d ", array_for_merge_sort[i]);
    }
    time_start();
    ParallelMergeSort(array_for_merge_sort, n);
    double merge_sort_time = time_stop();
    printf("MergeSort time: %f\n", merge_sort_time);
    time_start();
    qsort(&array_for_quick_sort[0], n, sizeof(int), CMP);
    double quick_sort_time = time_stop();
    printf("QuickSort time: %f\n", quick_sort_time);
    file1 = fopen("stats.txt", "ab");
    if(file1 == NULL)
        perror("unable to open the file");
    fprintf(file1, "%f %d %d %d", merge_sort_time, n, m, P);
    fprintf(file2,"\n");
    for(int i = 0; i < n; i++)
    {
        fprintf(file2, "%d ", array_for_merge_sort[i]);
    }
    fclose(file1);
    fclose(file2);

    free(array_for_merge_sort);
    free(array_for_quick_sort);
    return 0;
}

