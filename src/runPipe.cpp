#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
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
    puts("Usage: ./runPipe <yml-file>");
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
bool loadFile(vector<job_desc> &jobs, vector<pipe_desc> &pipes, char* fileName){
  if (!loadFromYAML(jobs, pipes, fileName)) {
    string errorMessage = "An error ocurred while trying to load and parse";
    errorMessage += " the specified YAML file";
    printf("%s\n", errorMessage.c_str());
    return false;
  }
  return true;
}

/**
    Prints a message with the result of the execution of a process. It can be
    either successful or not.
    @param success true if is a success message, false otherwise.
    @param jobName Name of the executed job.
    @param code Response code given by the status or signal.
 */
void printResult(bool success, string jobName, int code) {
  printf("\n## %s finished ", jobName.c_str());
  if (success) printf("successfully ##\n");
  else printf("unsuccessfully (Err: %d) ##\n", code);
}

int
replaceFileDescriptor(int descriptor, const char *file, int flags) {
  int originalDescriptor = dup(descriptor);
  close(descriptor);
  open(file, flags);
  return originalDescriptor;
}

void
executeProcess(int *descriptors1, int *descriptors2, int jobIndex,
               int jobsCount) {
  close(descriptors1[0]);
  close(descriptors1[1]);
  close(descriptors2[0]);
  close(descriptors2[1]);
  printf("Soy el proceso id %d y hay %d\n", jobIndex, jobsCount);
}

void
initializePipe(pipe_desc pipeToInit) {
  int originalInput, originalOutput;
  if (pipeToInit.input != STD_IN) {
    originalInput = replaceFileDescriptor(STDIN_FILENO,
                                          pipeToInit.input.c_str(), O_RDWR);
  }
  if (pipeToInit.output != STD_OUT) {
    originalOutput = replaceFileDescriptor(STDOUT_FILENO,
                                           pipeToInit.output.c_str(),
                                           O_RDWR | O_CREAT);
  }
  // FORKS Y la vuelta aquí
  // Creo las dos tuberías en el proceso padre, debería cerrarla en el hijo
  // que no vaya a utilizar.
  int descriptors1[2], descriptors2[2];
  pipe(descriptors1);
  pipe(descriptors2);
  pid_t lastChild;
  int jobsCount = pipeToInit.jobsIndexes.size();
  for (int i = 0; i < jobsCount; ++i) {
    lastChild = fork();
    if (lastChild == -1) {
      // TODO Fork failed
    }
    else if (lastChild == 0) {
      executeProcess(descriptors1, descriptors2, pipeToInit.jobsIndexes[i],
                     jobsCount);
      exit(EXIT_SUCCESS);
    }
    else {
      // Soy el padre.
    }
  }
  close(descriptors1[0]);
  close(descriptors1[1]);
  close(descriptors2[0]);
  close(descriptors2[1]);
  int status;
  waitpid(lastChild, &status, 0);
  /*int x;
  scanf("%d", &x);
  printf("%d\n", x + 10);*/
}

int
main(int argc, char **argv) {
  // Contains all data read and parsed from YAML file.
  job_desc job;
  vector <job_desc> jobs;
  vector <pipe_desc> pipes;
  if (!checkArgs(argc)) return 0;
  if (!loadFile(jobs, pipes, argv[1])) return 0;
  vector <pid_t> pipesCreators;
  bool parent = true;
  for (int i = 0; i < pipes.size() && parent; ++i) {
    pid_t child = fork();
    if (child != 0) pipesCreators.push_back(child);
    else {
      initializePipe(pipes[i]);
      parent = false;
    }
  }
  for (int i = 0; i < pipesCreators.size() && parent; ++i) {
    int status;
    waitpid(pipesCreators[i], &status, 0);
  }
  return 0;
  /*// Process id to do fork.
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
      printResult(false, job.name, errno);
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
            printResult(false, job.name, code);
          }
          else printResult(true, job.name, code);
        }
        // Returns true if the child process was terminated by a signal.
        else if (WIFSIGNALED(status)) {
          // Returns the number of the signal that caused the child process to
          // terminate.
          int signal_code = WTERMSIG(status);
          printResult(false, job.name, signal_code);
        }
      }
      break;
  }
  return 0;*/
}
