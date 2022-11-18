#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
#define SEM_REQ_CONS "req_cons"
#define SEM_REQ_PROD "req_prod"
#define SEM_LINE_CONS "line_cons"
#define SEM_LINE_PROD "line_prod"
#define SH_POS "sh_pos"
#define SH_LINE "sh_line"
#define MAX_CHARS 100

int status;

int main(int argc, char* argv[]) {
  const unsigned int N = atoi(argv[1]);
  int num_of_lines = atoi(argv[2]);

  // Open all named semaphores created from producer file
  sem_t* req_cons = sem_open(SEM_REQ_CONS, O_RDWR);
  sem_t* req_prod = sem_open(SEM_REQ_PROD, O_RDWR);

  sem_t* line_cons = sem_open(SEM_LINE_CONS, O_RDWR);
  sem_t* line_prod = sem_open(SEM_LINE_PROD, O_RDWR);

  // Open named shared memory created from producer and map it
  int shm_line = shm_open(SH_LINE, O_RDWR, 0666);
  char* ptr_shared_line = (char*)mmap(0, MAX_CHARS, PROT_READ | PROT_WRITE,
                                      MAP_SHARED, shm_line, 0);

  int shm_pos = shm_open(SH_POS, O_RDWR, 0666);
  char* ptr_shared_pos = (char*)mmap(0, sizeof(int), PROT_READ | PROT_WRITE,
                                     MAP_SHARED, shm_pos, 0);

  // getpid() is different for each child process
  srand(time(NULL) + getpid());
  double sum = 0;
  clock_t t;
  for (int i = 0; i < N; i++) {
    // Request: chosing a random number and updating shared memory
    sem_wait(req_cons);
    int random = rand() % (num_of_lines - 1);
    char* buffer = malloc(sizeof(int));
    snprintf(buffer, sizeof(int), "%d", random);
    strcpy(ptr_shared_pos, buffer);
    free(buffer);
    printf("Request: \"Get line %s from file\"\n", ptr_shared_pos);
    t = clock();
    sem_post(req_prod);

    // Receiving requested line
    sem_wait(line_prod);
    t = clock() - t;
    double time_taken = ((double)t) / CLOCKS_PER_SEC;
    sum += time_taken;
    printf("Client received line: %s\n", ptr_shared_line);
    sem_post(line_cons);
  }
  printf("Average response time %f\n", sum / N);

  // Close all semaphores. Don't unlink any semaphores or shared memory
  sem_close(line_prod);
  sem_close(line_cons);
  sem_close(req_cons);
  sem_close(req_prod);
  return 0;
}
