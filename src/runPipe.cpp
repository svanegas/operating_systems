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
#include <set>
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
bool loadFile(vector<job_desc> &jobs, vector<pipe_desc> &pipes,
              char* fileName, set <int> &assignedJobs) {
  if (!loadFromYAML(jobs, pipes, fileName, assignedJobs)) {
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
void printResult(bool success, string pipeName, int code) {
  printf("## %s finished ", pipeName.c_str());
  if (success) printf("successfully ##\n");
  else printf("unsuccessfully (Err: %d) ##\n", code);
}

int replaceFileDescriptor(int descriptor, const char *file, int flags) {
  int originalDescriptor = dup(descriptor);
  if (originalDescriptor == ERROR_OCURRED) return ERROR_OCURRED;
  if (close(descriptor) == ERROR_OCURRED) return ERROR_OCURRED;
  if (open(file, flags) == ERROR_OCURRED) return ERROR_OCURRED;
  return originalDescriptor;
}

bool isOpen(int fd) {
  return fd != FD_CLOSED;
}

void closeFileDescriptor(int &fd) {
  close(fd);
  fd = FD_CLOSED;
}

bool setupPipeDescriptor(int pipeDescriptor, int fdToReplace) {
  if (dup2(pipeDescriptor, fdToReplace) == ERROR_OCURRED) return false;
  if (close(pipeDescriptor) == ERROR_OCURRED) return false;
  return true;
}

bool waitForChildren(vector <pid_t> &children) {
  for (int i = 0; i < children.size(); ++i) {
    pid_t pid = children[i];
    int status;
    if (waitpid(pid, &status, 0) == pid) {
      // Returns true if the child terminated normally.
      if (WIFEXITED(status)) {
        // Returns the exit status of the child.
        int code = WEXITSTATUS(status);
        if (code != EXIT_SUCCESS) {
          errno = code;
          return false;
        }
      }
      // Returns true if the child process was terminated by a signal.
      else if (WIFSIGNALED(status)) {
        // Returns the number of the signal that caused the child process to
        // terminate.
        int signalCode = WTERMSIG(status);
        errno = signalCode;
        return false;
      }
    }
  }
  return true;
}

pipe_desc buildDefaultPipe(int jobCount, set <int> &assignedJobs) {
  pipe_desc defaultPipe;
  defaultPipe.name = DEFAULT_PIPE;
  defaultPipe.input = STD_IN;
  defaultPipe.output = STD_OUT;
  for (int i = 0; i < jobCount; ++i) {
    if (assignedJobs.count(i) == 0) {
      defaultPipe.jobsIndexes.push_back(i);
    }
  }
  return defaultPipe;
}

bool executeProcess(int descriptor[2][2], int jobPosition, int jobsCount,
                    job_desc job) {
  // Even processes don't read from first pipe descriptor neither write to
  // second pipe descriptor.
  if (jobPosition % 2 == 0) {
    closeFileDescriptor(descriptor[FIRST_PIPE_FD][STDIN_FILENO]);
    closeFileDescriptor(descriptor[SECOND_PIPE_FD][STDOUT_FILENO]);
  }
  // Odd processes don't read from second pipe descriptor neither write to
  // first pipe descriptor.
  else if (jobPosition % 2 == 1) {
    closeFileDescriptor(descriptor[FIRST_PIPE_FD][STDOUT_FILENO]);
    closeFileDescriptor(descriptor[SECOND_PIPE_FD][STDIN_FILENO]);
  }
  // First process is supposed to read from second pipe descriptor because it
  // is even, but it shouldn't read from a pipe descriptor.
  if (jobPosition == 0) {
    closeFileDescriptor(descriptor[SECOND_PIPE_FD][STDIN_FILENO]);
  }
  // Last process is supposed to write to a pipe descriptor because it
  // is even or odd, but it shouldn't write to a pipe descriptor.
  if (jobPosition == jobsCount - 1) {
    // If last is even, it was supposed to write in first pipe descriptor.
    if (jobPosition % 2 == 0) {
      closeFileDescriptor(descriptor[FIRST_PIPE_FD][STDOUT_FILENO]);
    }
    // If last is odd, it was supposed to write in second pipe descriptor.
    else closeFileDescriptor(descriptor[SECOND_PIPE_FD][STDOUT_FILENO]);
  }

  // Check which file descriptors are open and replace the respective ones.
  for (int descriptorIndex = 0; descriptorIndex < 2; ++descriptorIndex) {
    for (int fdIndex = 0; fdIndex < 2; ++fdIndex) {
      if (isOpen(descriptor[descriptorIndex][fdIndex])) {
        if (!setupPipeDescriptor(descriptor[descriptorIndex][fdIndex], fdIndex))
        {
          return false;
        }
      }
    }
  }
  /*if (isOpen(descriptor[FIRST_PIPE_FD][STDIN_FILENO])) {
    setupPipeDescriptor(descriptor[FIRST_PIPE_FD][STDIN_FILENO]);
  }
  if (isOpen(descriptor[FIRST_PIPE_FD][STDOUT_FILENO])) {
    setupPipeDescriptor(descriptor[FIRST_PIPE_FD][STDOUT_FILENO]);
  }
  if (isOpen(descriptor[SECOND_PIPE_FD][STDIN_FILENO])) {
    setupPipeDescriptor(descriptor[SECOND_PIPE_FD][STDIN_FILENO]);
  }
  if (isOpen(descriptor[SECOND_PIPE_FD][STDOUT_FILENO])) {
    setupPipeDescriptor(descriptor[SECOND_PIPE_FD][STDOUT_FILENO]);
  }*/

  // Array that contains: [job name, args..., NULL].
  char *jobArgs [job.args.size() + 2];
  jobArgs[0] = (char *) job.exec.c_str();
  for (int i = 1; i <= job.args.size(); ++i) {
    jobArgs[i] = (char *) job.args[i - 1].c_str();
  }
  // "The list of arguments must be terminated by a NULL pointer, and, since
  // these are variadic functions, this pointer must be cast (char *) NULL."
  jobArgs[job.args.size() + 1] = NULL;

  if (execvp(jobArgs[0], jobArgs) == ERROR_OCURRED) return false;
  else return true;
  /*closeFileDescriptor(descriptor[FIRST_PIPE_FD][STDIN_FILENO]);
  closeFileDescriptor(descriptor[FIRST_PIPE_FD][STDOUT_FILENO]);
  closeFileDescriptor(descriptor[SECOND_PIPE_FD][STDIN_FILENO]);
  closeFileDescriptor(descriptor[SECOND_PIPE_FD][STDOUT_FILENO]);
  printf("%s\n", job.exec.c_str());*/
}

bool initializePipe(pipe_desc pipeToInit, vector <job_desc> &jobs) {
  int originalInput, originalOutput;
  if (pipeToInit.input != STD_IN) {
    originalInput = replaceFileDescriptor(STDIN_FILENO,
                                          pipeToInit.input.c_str(), O_RDWR);
    if (originalInput == ERROR_OCURRED) return false;
  }
  if (pipeToInit.output != STD_OUT) {
    originalOutput = replaceFileDescriptor(STDOUT_FILENO,
                                           pipeToInit.output.c_str(),
                                           O_RDWR | O_CREAT);
    if (originalOutput == ERROR_OCURRED) return false;
  }
  // FORKS Y la vuelta aquí
  // Creo las dos tuberías en el proceso padre, debería cerrarla en el hijo
  // que no vaya a utilizar.
  int descriptor[2][2];
  if (pipe(descriptor[FIRST_PIPE_FD]) == ERROR_OCURRED) return false;
  if (pipe(descriptor[SECOND_PIPE_FD]) == ERROR_OCURRED) return false;

  //vector <pid_t> childJobs;
  pid_t lastJob;
  int jobsCount = pipeToInit.jobsIndexes.size();
  for (int i = 0; i < jobsCount; ++i) {
    pid_t currentChild;
    int jobIndex;
    job_desc currentJob;
    switch (currentChild = fork()) {
      case ERROR_OCURRED:
        return false;
      case 0:
        jobIndex = pipeToInit.jobsIndexes[i];
        currentJob = jobs[jobIndex];
        if (!executeProcess(descriptor, i, jobsCount, currentJob)) exit(errno);
        else exit(EXIT_SUCCESS);
        break;
      default:
        //childJobs.push_back(currentChild);
        lastJob = currentChild;
        break;
    }
  }
  closeFileDescriptor(descriptor[FIRST_PIPE_FD][STDIN_FILENO]);
  closeFileDescriptor(descriptor[FIRST_PIPE_FD][STDOUT_FILENO]);
  closeFileDescriptor(descriptor[SECOND_PIPE_FD][STDIN_FILENO]);
  closeFileDescriptor(descriptor[SECOND_PIPE_FD][STDOUT_FILENO]);

  vector <pid_t> lastJobInVector(1, lastJob);
  return waitForChildren(lastJobInVector);
}

void analyzePipeResults(pid_t exitedPipeId, string pipeName) {
  int status;
  if (waitpid(exitedPipeId, &status, 0) == exitedPipeId) {
    // Returns true if the child terminated normally.
    if (WIFEXITED(status)) {
      // Returns the exit status of the child.
      int code = WEXITSTATUS(status);
      if (code != EXIT_SUCCESS) {
        printResult(false, pipeName, code);
      }
      else printResult(true, pipeName, code);
    }
    // Returns true if the child process was terminated by a signal.
    else if (WIFSIGNALED(status)) {
      // Returns the number of the signal that caused the child process to
      // terminate.
      int signalCode = WTERMSIG(status);
      printResult(false, pipeName, signalCode);
    }
  }
}

pid_t forkToCreatePipe(pipe_desc pipeToCreate, vector <job_desc> &jobs) {
  pid_t child;
  printf("## Output %s ##\n", pipeToCreate.name.c_str());
  switch (child = fork()) {
    case ERROR_OCURRED:
      // An error ocurred while trying to fork.
      printResult(false, pipeToCreate.name, errno);
      break;
    case 0:
      if (!initializePipe(pipeToCreate, jobs)) exit(errno);
      else exit(EXIT_SUCCESS);
    break;
  }
  return child;
}

int
main(int argc, char **argv) {
  // Contains all data read and parsed from YAML file.
  job_desc job;
  vector <job_desc> jobs;
  vector <pipe_desc> pipes;
  set <int> assignedJobs;
  if (!checkArgs(argc)) return 0;
  if (!loadFile(jobs, pipes, argv[1], assignedJobs)) return 0;
  vector <pid_t> pipeCreators;
  for (int i = 0; i < pipes.size(); ++i) {
    pid_t child = forkToCreatePipe(pipes[i], jobs);
    if (child > 0) pipeCreators.push_back(child);
  }
  for (int i = 0; i < pipeCreators.size(); ++i) {
    pipe_desc exitedPipe = pipes[i];
    pid_t exitedPipeId = pipeCreators[i];
    int status;
    analyzePipeResults(exitedPipeId, exitedPipe.name);
  }

  pipe_desc defaultPipe = buildDefaultPipe(jobs.size(), assignedJobs);
  pid_t defaultPipeChild = forkToCreatePipe(defaultPipe, jobs);
  analyzePipeResults(defaultPipeChild, defaultPipe.name);

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
