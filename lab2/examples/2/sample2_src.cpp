#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <assert.h>
#include <signal.h>

using namespace std;

int main(int argc, char **argv) {
  sleep(atoi(argv[1]));
  printf("I just waited %s seconds\n", argv[1]);
  exit(EXIT_SUCCESS);
}
