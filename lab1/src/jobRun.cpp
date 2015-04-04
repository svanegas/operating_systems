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

/**
    Checks if the number of console arguments are correct. In case they are
    wrong a message is printed.
    @param argc Number of arguments of the program.
    @return true if are correct, false otherwise.
 */
bool checkArgs(int argc) {
  if (argc != 2) {
    puts("Usage: ./jobRun <yml-file>");
    return false;
  }
  return true;
}

/**
    Loads a job description from a YAML file specified in parameters.
    @param destination Reference to job_desc structure to be filled.
    @param fileName Name of the YAML file.
    @return true if the given job_desc was filled successfully, false
            otherwhise.
 */
bool loadFile(job_desc &destination, char* fileName) {
  if (!destination.loadFromYAML(destination, fileName)) {
    puts("Could not open specified YAML file");
    return false;
  }
  return true;
}

/**
    Creates a copy of the respective file descriptors for input, output
    and error streams. If those are specified as standard nothing is done.
    @param job Reference to job_desc containing the input, output and error
               file names.
    @param inFile Pointer to input file.
    @param outFile Pointer to output file.
    @param errFile Pointer to error file.
    @return true if the descriptors were succesfully copied, false
            otherwhise.
 */
bool setupStreamFiles(job_desc &job, FILE *inFile, FILE *outFile,
                      FILE *errFile) {
  if (job.input != STD_IN) {
    if ((inFile = fopen(job.input.c_str(), READ_MODE.c_str())) == NULL) {
      return false;
    }
    // dup2(oldfd, newfd) makes newfd be the copy of oldfd, closing newfd first
    // if necessary.
    if (dup2(fileno(inFile), STDIN_FILENO) == ERROR_OCURRED) return false;
    close(fileno(inFile));
  }
  if (job.output != STD_OUT) {
    if ((outFile = fopen(job.output.c_str(), WRITE_MODE.c_str())) == NULL) {
      return false;
    }
    if (dup2(fileno(outFile), STDOUT_FILENO) == ERROR_OCURRED) return false;
    close(fileno(outFile));
  }
  if (job.error != STD_ERR) {
    if ((errFile = fopen(job.error.c_str(), WRITE_MODE.c_str())) == NULL) {
      return false;
    }
    if (dup2(fileno(errFile), STDERR_FILENO) == ERROR_OCURRED) return false;
    close(fileno(errFile));
  }
  return true;
}

/**
    Prints a message with the result of the execution of a process. It can be
    either successful or not.
    @param success true if is a success message, false otherwise.
    @param jobName Name of the executed job.
    @param code Response code given by the status or signal.
    @param description Message to be displayed when it is a failure response.
 */
void printResult(bool success, string jobName, int code, char * description) {
  printf("\n## %s finished ", jobName.c_str());
  if (success) printf("successfully ##\n");
  else printf("unsuccessfully (Err: %d - %s) ##\n", code, description);
}

int main (int argc, char **argv) {
  // Contains all data read and parse from YAML file.
  job_desc job;
  if (!checkArgs(argc)) return 0;
  if (!loadFile(job, argv[1])) return 0;
  // Process id to do fork.
  pid_t pid;
  // Pointers to job input, output and error files.
  FILE *inFile, *outFile, *errFile;
  // Status returned by the child process, contains either success or failure
  // code.
  int status;
  // Array that contains: [job name, args..., NULL].
  char *jobArgs [job.args.size() + 2];
  jobArgs[0] = (char *) job.exec.c_str();
  for (int i = 1; i <= job.args.size(); ++i) {
    jobArgs[i] = (char *) job.args[i - 1].c_str();
  }
  // "The list of arguments must be terminated by a NULL pointer, and, since
  // these are variadic functions, this pointer must be cast (char *) NULL."
  jobArgs[job.args.size() + 1] = NULL;
  printf("## Running %s ##\n", job.name.c_str());
  switch (pid = fork()) {
    // An error occurred while trying to fork.
    case ERROR_OCURRED:
      printResult(false, job.name, errno, strerror(errno));
      break;
    // Child block is pid = 0.
    case 0:
      // If could not setup the stream files exit with the error number.
      if (!setupStreamFiles(job, inFile, outFile, errFile)) exit(errno);
      // execvp replaces the current child process image with a new one.
      // It returns (-1) only if an error has ocurred, setting the errno
      // with the respective error.
      else if (execvp(jobArgs[0], jobArgs) == ERROR_OCURRED) exit(errno);
      else exit(EXIT_SUCCESS);
      break;
    // Parent block is pid = childpid.
    default:
      // Waitpid is used to wait for state changes in a child of the calling
      // process and obtain information about the child whose state has changed.
      if (waitpid(pid, &status, 0) == pid) {
        // Returns true if the child terminated normally.
        if (WIFEXITED(status)) {
          // Returns the exit status of the child.
          int code = WEXITSTATUS(status);
          if (code != EXIT_SUCCESS) {
            printResult(false, job.name, code, strerror(code));
          }
          else printResult(true, job.name, 0, NULL);
        }
        // Returns true if the child process was terminated by a signal.
        else if (WIFSIGNALED(status)) {
          // Returns the number of the signal that caused the child process to
          // terminate.
          int signal_code = WTERMSIG(status);
          printResult(false, job.name, signal_code, strsignal(signal_code));
        }
      }
      break;
  }
  return 0;
}
