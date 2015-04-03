#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <cstring>
#include <errno.h>
#include <string>
#include "jobdesc.h"

using namespace std;

#define ERROR_OCURRED -1

bool checkArgs(int argc) {
  if (argc != 2) {
    puts("Usage: ./jobRun <yml-file>");
    return false;
  }
  return true;
}

bool loadFile(job_desc &destination, char* fileName) {
  if (!destination.loadFromYAML(destination, fileName)) {
    puts("Could not open specified YAML file");
    return false;
  }
  return true;
}

int main (int argc, char **argv) {
  job_desc job;
  if (!checkArgs(argc)) return 0;
  if (!loadFile(job, argv[1])) return 0;
  pid_t pid;
  int status;
  char *jobArgs [job.args.size() + 2];
  jobArgs[0] = (char *) job.exec.c_str();
  for (int i = 1; i <= job.args.size(); ++i) {
    jobArgs[i] = (char *) job.args[i-1].c_str();
  }
  // "The list of arguments must be terminated by a NULL pointer, and, since
  // these are variadic functions, this pointer must be cast (char *) NULL."
  jobArgs[job.args.size() + 1] = NULL;
  printf("## Running %s ##\n", job.name.c_str());
  switch (pid = fork()) {
    case ERROR_OCURRED:
      exit(errno);
      break;
    case 0: // Child block
      if (execvp(jobArgs[0], jobArgs) == ERROR_OCURRED) exit(errno);
      else exit(EXIT_SUCCESS);
      break;
    default: // Parent block
      //Waitpid is used to wait for state changes in a child of the calling 
      //process and obtain information about the child whose state has changed. 
      if (waitpid(pid, &status, 0) == pid) {
        if (WIFEXITED(status)) {
          printf("\n## %s finished ", job.name.c_str());
          int code = WEXITSTATUS(status);
          if (code != EXIT_SUCCESS) {
            printf("unsuccessfully (Err: %d - %s) ##\n", code, strerror(code));
          }
          else {
            printf("successfully ##\n");
          }
        }
        else if (WIFSIGNALED(status)) {
          printf("unsuccessfully (Err: %d) ##\n", WTERMSIG(status));
        }
      }
      break;
  }
  return 0;
}
