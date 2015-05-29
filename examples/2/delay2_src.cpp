#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <assert.h>
#include <signal.h>

using namespace std;

int main() {
  sleep(5);
  puts("delay 5 secs");
  exit(EXIT_SUCCESS);
}
