#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <cstring>
#include <errno.h>
#include <string>

using namespace std;

#define ERROR_OCURRED -1

int main() {
  pid_t pid;
  int status;
  string jobName = "test-job";
  char *args[5];
  args[0] = (char *) "echo";
  args[1] =  (char *) "-n";
  args[2] =  (char *) "this is a test";
  args[3] = NULL;
  printf("## Running %s ##\n", jobName.c_str());
  switch (pid = fork()) {
    case ERROR_OCURRED:
      exit(errno);
      break;
    case 0: // Child block
      if (execvp(args[0], args) == ERROR_OCURRED) exit(errno);
      else exit(EXIT_SUCCESS);
      break;
    default: // Parent block
      if (waitpid(pid, &status, 0) == pid) {
        if (WIFEXITED(status)) {
          printf("\n## %s finished ", jobName.c_str());
          int code = WEXITSTATUS(status);
          if (code != EXIT_SUCCESS) {
            printf("unsuccessfully (Err: %d - %s) ##\n", code, strerror(code));
          }
          else {
            printf("successfully ##\n");
          }
        }
      }
      break;
  }
  return 0;
}
