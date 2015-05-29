#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <assert.h>
#include <signal.h>

using namespace std;

int main(int argc, char **argv) {
  /*string message;
  for (int i = 0; i < atoi(argv[1]); ++i) {
    cin >> message;
    printf("message # %d = %s\n", i + 1, message.c_str());
  }
  sleep(2),
  cerr << "This is an error message." << endl;
  exit(EXIT_SUCCESS);*/
  puts("Estoy waiting for it");
  sleep(2);
  printf("Hola gente %s\n", argv[1]);
  exit(EXIT_SUCCESS);
}
