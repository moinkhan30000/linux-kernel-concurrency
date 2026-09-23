//
// Created by LENOVO on 16/02/2025.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <stdio.h>


#include <sys/wait.h>

#define MAX_K = 10000
#define MAX_N = 20

int compare(const void *a, const void *b) {
    return (*(long int*)a - *(long int*)b);
}

void heapify (int root, long int* array, int size){
    int child = 2 * root + 1;
    if (child < size){
        int right_child = child +1;
        if ((right_child < size) && (array[right_child] < array[child]))
            child = right_child;
        if (array[root] > array[child]) {
            long int temp = array[root];
            array[root] = array[child];
            array[child] = temp;

            heapify(child, array,size);
        }
    }
}

void build_heap(int root, long int* array, int size) {
    int left = 2*root+1;
    int right = left+1;
    if(left<size){
        build_heap(left,array,size);
    }
    if(right<size){
        build_heap(right,array,size);
    }
    int parent = (root - 1) / 2;
    if(array[parent]>array[root]){
        long int temp = array[parent];
        array[parent]=array[root];
        array[root]=temp;
    }
}

void heap_insert(long int value, long int* array, int size){
    array[size] = value;



    int parent = (size - 1)/2;
    while ( (size > 0) && (array[size] < array[parent]) ) {
        long int temp = array[parent];
        array[parent] = array[size];
        array[size] = temp;

        size = parent;
        parent = (size - 1)/2;
    }
    ++size;
}
void readWriteFile(const char* filename, int N, int** sizeOfFile, char *fileNames[N])
{
    int index = 1;

    int **files = (int **)malloc(N * sizeof (int*));
    *sizeOfFile = (int *)malloc(N * sizeof (int));

    for(int i = 0; i < N; i++){
        files[i] = NULL;
    }

    for(int i = 0; i < N; i++){
        (*sizeOfFile)[i] = 0;
    }


    int *fileValues = NULL;


    FILE* f = fopen(filename,"r");
    int number = 0;
    if(f==NULL)
    {
        printf("Error opening File");
        return;
    }
    while (fscanf(f, "%d", &number) != EOF)
    {
        int file = (index - 1) % N;

        (*sizeOfFile)[file] += 1;

        files[file] = (int *) realloc(files[file], (*sizeOfFile)[file] * sizeof (int));

        files[file][(*sizeOfFile)[file] - 1] = number;

        index++;
    }


    fclose(f);




    for(int i = 0; i < N; i++){


        fileNames[i] = (char*) malloc(50 * sizeof(char));


        sprintf(fileNames[i], "File-%d.txt", i + 1);



        FILE* fd = fopen(fileNames[i],"w");

        if(!fd) {
            perror("Failed to open file");
            exit(EXIT_FAILURE);
        }

        for(int j = 0; j < (*sizeOfFile)[i]; j++){

            fprintf(fd, "%d\n", files[i][j]);

        }

        fclose(fd);
    }
    for (int i = 0; i < N; i++) {
        free(files[i]);  // Free each row first
    }
    free(files);
}


int main(int argc, char *argv[]){
    int K = -1, N = -1;
    char input_file[100] = "";
    char output_file[100] = "";
    int opt;
    while ((opt = getopt(argc, argv, "t:c:i:o:")) != -1) {
        switch (opt) {
            case 't':
                K = atoi(optarg);
                break;
            case 'c':
                N = atoi(optarg);
                break;
            case 'i':
                strncpy(input_file, optarg, sizeof(input_file) - 1);
                break;
            case 'o':
                strncpy(output_file, optarg, sizeof(output_file) - 1);
                break;
            default:
                return EXIT_FAILURE;
        }
    }

    int* fileNumber = NULL;
    char* names[N];
    readWriteFile(input_file, N, &fileNumber, names);
    clock_t start = clock();
    char* name= "shr_mem";
    int shm = shm_open(name, O_CREAT | O_RDWR, 0420);
    ftruncate(shm, N * K * sizeof(long int));
    long int *shm_ptr = mmap(0, N * K * sizeof(long int), PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);

    for (int i = 0; i < N; i++) {
        pid_t child_process = fork();
        if (child_process < 0){
            printf("Child could not obtained \n");
            exit(0);
        }
        else if(child_process ==0){

            FILE* f = fopen(names[i],"r");
            int count=0;
            long int number;
            long int heap[K];

            while (fscanf(f, "%ld\n", &number) != EOF){
                if(count<K){
                    heap_insert(number, heap, count);
                    count++;
                }
                else{
                    if( number > heap[0]){
                        heap[0] =  number;
                        heapify(0,heap,K);
                    }
                }
            }
            memcpy(&shm_ptr[i * K], heap, K * sizeof(long int));
            exit(0);

        }
    }
    for (int i = 0; i <N; i++) {
        wait(NULL);
    }
    long int final_values[N * K];
    memcpy(final_values, shm_ptr, N * K * sizeof(long int));

    qsort(final_values, N * K, sizeof(long int), compare);

    clock_t end = clock();
    for(int i=0; i<N; i++)
    {
        remove(names[i]);
        free(names[i]);

    }

    shm_unlink(name);
    free(fileNumber);
    FILE* fd = fopen(output_file,"w");
    if(!fd) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    for(int j = 0; j < K; j++){

        fprintf(fd, "%ld\n", final_values[N*K-j-1]);

    }

    fclose(fd);

    double elapsed_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Execution Time: %f seconds\n", elapsed_time);


    return 0;

}