#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <omp.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <sys/wait.h>


int sudokuTable[9][9];
int columns = 0;
int rows = 0;
int array_of_checks[9] = {
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  9,
};

int check_row(int row) {
  if (row < 0 || 8 < row) {
    return -1;
  }
  printf("Hilo que ve la fila: %ld\n", syscall(SYS_gettid));

  for (int i = 0; i < 9; i++) {
    for (int j = 0; j < 9; j++) {
      if (sudokuTable[row][i] == array_of_checks[j]) {
        array_of_checks[j] = -1;
        break;
      }
      if (j == 8) {
        return -1;
      }
    }
  }
  return 0;
}

int check_column(int column) {
  if (column < 0 || 8 < column) { return -1; }
  printf("Columna %d, thread en ejecucion: %ld\n", column, syscall(SYS_gettid));
  for (int i = 0; i < 9; i++) {
    for (int j = 0; j < 9; j++) {
      if (sudokuTable[i][column] == array_of_checks[j]) {
        array_of_checks[j] = -1;
        break;
      }
      if (j == 8) {
        return -1;
      }
    }
  }
  return 0;
}

void* check_all_columns(void *arg) {
  printf("Hilo que ve columnas: %ld\n", syscall(SYS_gettid));
  int i, check;
  omp_set_num_threads(8);
  omp_set_nested(1);
  #pragma omp parallel for ordered
  for (i = 0; i < 8; i++) {
    check = check_column(i);
    if (check == -1) {
      columns = -1;
    }
  }
  pthread_exit(0);
}

int check_group(int row, int column) {
  if (column != 0 && column != 3 && column != 6 ) {
    return -1;
  }
  if (row != 0 && row != 3 && row != 6 ) {
    return -1;
  }
  for (int i = 0; i < 9; i++) {
    int x = row + (i / 3);
    int y = column + (i % 3);
    for (int j = 0; j < 9; j++) {
      if (sudokuTable[x][y] == array_of_checks[j]) {
        array_of_checks[j] = -1;
        return -1;
      }
      if (j == 8) {
        return -1;
      }
    }
  }
  return 0;
}

int main(int argc, char** argv) {
  char* path = argv[1];
  int readFile;
  omp_set_num_threads(4);
  pthread_t thread;

  readFile = open(path, O_RDONLY, 0666);\
  struct stat sb;
  if (fstat(readFile, &sb) == -1) {
    perror("Fallo en archivo");
  }

  char* opened_file = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, readFile, 0);
  for (int i = 0; i < 81; i++) {
    int x = i / 9;
    int y = i % 9;
    sudokuTable[x][y] = opened_file[i] - '0';
  }

  if (fork() == 0) {
    char parentPID[10];
    sprintf(parentPID, "%d", (int)(getppid()));
    execlp("ps", "ps", "-p", parentPID, "-lLf", NULL);
  } else {
    pthread_create(&thread, NULL, check_all_columns, NULL);
    pthread_join(thread, NULL);

    printf("Hilo que ejecuta main: %ld\n", syscall(SYS_gettid));

    wait(NULL);
    int check;

    omp_set_num_threads(8);
    omp_set_nested(1);

  
    #pragma omp parallel for ordered schedule (dynamic)
    for (int i = 0; i < 8; i++) {
      check = check_row(i);
      if (check == -1) {
        rows = -1;
      }
    }

    if (fork() == 0) {
      char parentPID[10];
      sprintf(parentPID, "%d", (int)(getppid()));
      execlp("ps", "ps", "-p", parentPID, "-lLf", NULL);
    } else {
      wait(NULL);
    }
  }
  pthread_exit(0);
}
