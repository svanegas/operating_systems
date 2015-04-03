#include <cstdio>
#include <stdlib.h>
#include <unistd.h>

using namespace std;

int main(int argc, char **argv) {
  for (int i = 0; i < atoi(argv[1]); ++i) {
    printf("%d\n", i);
  }
  sleep(2),
  exit(EXIT_SUCCESS);
}
