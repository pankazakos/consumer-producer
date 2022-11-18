#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
#define CHILD_PROGRAM "./consumer"
#define SEM_REQ_CONS "req_cons"
#define SEM_REQ_PROD "req_prod"
#define SEM_LINE_CONS "line_cons"
#define SEM_LINE_PROD "line_prod"
#define SH_POS "sh_pos"
#define SH_LINE "sh_line"
#define MAX_CHARS 100

int CountLines(char*);
char* GetLine(char*, char*, int);

int main(int argc, char* argv[]) {
  char* X = argv[1];
  const unsigned int K = atoi(argv[2]);
  const unsigned int N = atoi(argv[3]);

  // open 4 named semaphores to control sychronization and mutual exclusion of
  // server and client
  sem_t* req_cons = sem_open(SEM_REQ_CONS, O_CREAT | O_EXCL, SEM_PERMS, 1);
  if (req_cons == SEM_FAILED) {
    perror("req_cons: sem_open(3) failed");
    exit(EXIT_FAILURE);
  }
  sem_t* req_prod = sem_open(SEM_REQ_PROD, O_CREAT | O_EXCL, SEM_PERMS, 0);
  if (req_prod == SEM_FAILED) {
    perror("req_prod: sem_open(3) failed");
    exit(EXIT_FAILURE);
  }
  sem_t* line_cons = sem_open(SEM_LINE_CONS, O_CREAT | O_EXCL, SEM_PERMS, 1);
  if (line_cons == SEM_FAILED) {
    perror("line_cons: sem_open(3) failed");
    exit(EXIT_FAILURE);
  }
  sem_t* line_prod = sem_open(SEM_LINE_PROD, O_CREAT | O_EXCL, SEM_PERMS, 0);
  if (line_prod == SEM_FAILED) {
    perror("line_prod: sem_open(3) failed");
    exit(EXIT_FAILURE);
  }

  // Count number of lines of given file
  int num_of_lines = CountLines(X);

  // open named shared memory for line and map it to multiple processes
  int shm_line = shm_open(SH_LINE, O_CREAT | O_RDWR, 0666);
  ftruncate(shm_line, MAX_CHARS);
  char* ptr_shared_line = (char*)mmap(0, MAX_CHARS, PROT_READ | PROT_WRITE,
                                      MAP_SHARED, shm_line, 0);

  // open named shared memory for the number of line and map it to multiple
  // processes
  int shm_pos = shm_open(SH_POS, O_CREAT | O_RDWR, 0666);
  ftruncate(shm_pos, sizeof(int));
  char* ptr_shared_pos = (char*)mmap(0, sizeof(int), PROT_READ | PROT_WRITE,
                                     MAP_SHARED, shm_pos, 0);

  // Make N and num_of_lines strings in order to pass as arguments in execl
  char* N_buffer = malloc(sizeof(int));
  snprintf(N_buffer, sizeof(int), "%d", N);
  char* num_of_lines_buffer = malloc(sizeof(int));
  snprintf(num_of_lines_buffer, sizeof(int), "%d", num_of_lines);

  char* line = malloc(MAX_CHARS);
  int pid;
  int status;
  for (int i = 0; i < K; i++) {
    pid = fork();
    // fork failed
    if (pid < 0) {
      perror("fork");
      exit(EXIT_FAILURE);
      // child(client): exec consumer program
    } else if (pid == 0) {
      if (execl(CHILD_PROGRAM, CHILD_PROGRAM, N_buffer, num_of_lines_buffer,
                NULL) < 0) {
        perror("execl(2) failed");
        exit(EXIT_FAILURE);
      }
      // parent(server)
    } else if (pid > 0) {
      for (int i = 0; i < N; i++) {
        // Server: Receiving request ptr_shared_pos
        sem_wait(req_prod);
        int request = atoi(ptr_shared_pos);
        sem_post(req_cons);

        // Server: Producing line according to request
        sem_wait(line_cons);
        strcpy(ptr_shared_line, GetLine(X, line, request));
        sem_post(line_prod);
      }
      // Gather all children processes
      printf("Child %d finished all requests\n\n\n", wait(&status));
    }
  }
  // free malloced pointers
  free(line);
  free(N_buffer);
  free(num_of_lines_buffer);

  // close and unlink all named semaphores
  if (sem_close(req_cons) < 0) perror("req_cons: sem_close(3) failed");
  sem_unlink(SEM_REQ_CONS);
  if (sem_close(req_prod) < 0) perror("req_prod: sem_close(3) failed");
  sem_unlink(SEM_REQ_PROD);
  if (sem_close(line_cons) < 0) perror("line_cons: sem_close(3) failed");
  sem_unlink(SEM_LINE_CONS);
  if (sem_close(line_prod) < 0) perror("line_prod: sem_close(3) failed");
  sem_unlink(SEM_LINE_PROD);

  // unlink named shared memory
  shm_unlink(SH_POS);
  shm_unlink(SH_LINE);
  return 0;
}

// File is opened and closed inside function
int CountLines(char* file) {
  FILE* fp;
  fp = fopen(file, "r");
  if (fp == NULL) {
    printf("Could not open file \"%s\"\n", file);
    return -1;
  }
  int count = 0;
  for (char c = getc(fp); c != EOF; c = getc(fp))
    if (c == '\n') count++;
  fclose(fp);
  return count;
}

// File is opened and closed inside function
char* GetLine(char* file, char* buffer, int num) {
  FILE* fp;
  fp = fopen(file, "r");
  if (fp == NULL) {
    printf("Could not open file \"%s\"\n", file);
    return NULL;
  }
  int counter = 0;
  while (fgets(buffer, MAX_CHARS, fp) != NULL) {
    if (counter == num) {
      fclose(fp);
      return buffer;
    } else {
      counter++;
    }
  }
  fclose(fp);
  return NULL;
}
