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

/**
  Redirects the given stream file descriptor to a file using custom flags and
  saving and returning a copy of the original stream.
  @param descriptor Stream file descriptor to be redirected.
  @param file Name of the file to place instead the stream.
  @param flags Flags to be used while opening the file.
  @return On success, a copy of the original file descriptor is returned. On
          error, -1 is returned, and errno is set appropriately.
 */
int redirectStreamToFile(int descriptor, const char *file, int flags) {
  int originalDescriptor = dup(descriptor);
  if (originalDescriptor == ERROR_OCURRED) return ERROR_OCURRED;
  if (close(descriptor) == ERROR_OCURRED) return ERROR_OCURRED;
  if (open(file, flags) == ERROR_OCURRED) return ERROR_OCURRED;
  return originalDescriptor;
}

/**
  Checks if a file descriptor is open. We say that a file descriptor is closed
  if it is set to -1
  @param fd File descriptor to check.
  @return true if the file descriptor is different from -1, false otherwise.
 */
bool isOpen(int fd) {
  return fd != FD_CLOSED;
}

/**
  Closes a file descriptor, seting it to -1.
  @param fd Reference to file descriptor to close.
 */
void closeFileDescriptor(int &fd) {
  close(fd);
  fd = FD_CLOSED;
}

/**
  Takes 'pipeDescriptor' and duplicates it to 'fdToReplace' slot, then closes
  the original 'pipeDescriptor'.
  @param pipeDescriptor Descriptor to duplicate into 'fdToReplace' slot, then
                        it is closed.
  @param fdToReplace Descriptor to be replaced by 'pipeDescriptor'.
  @return On success, returns true. On error, returns false and errno is set
          appropriately.
 */
bool setupPipeDescriptor(int pipeDescriptor, int fdToReplace) {
  if (dup2(pipeDescriptor, fdToReplace) == ERROR_OCURRED) return false;
  if (close(pipeDescriptor) == ERROR_OCURRED) return false;
  return true;
}

/**
  Receives a vector of processes ids and waits for each one of them to finish.
  @param children Reference to a vector of processes ids (pid_t).
  @return true if all processes exited successfully, false otherwise.
 */
bool waitForChildren(vector <pid_t> &children) {
  pid_t pid;
  for (int i = 0; i < children.size(); ++i) {
    pid = children[i];
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

/**
  Takes a jobCount, set of assigned jobs then creates and fills a pipe
  description (pipe_desc) with the jobs that doesn't occur in the set.
  @param jobCount Total number of existing jobs.
  @param assignedJobs Set containing which jobs were previously assigned to any
                      other pipe.
  @return A pipe description with DEFALUT_PIPE name ("default-pipe"), input and
          output as standard and a list of jobs that were not assigned to
          a pipe and should run in this default pipe.
 */
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

/*bool executeProcess(int descriptor[2][2], int jobPosition, int jobsCount,
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
  }*/

  /** COMMENTED **/
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
  /** END COMMENTED **/

  // Array that contains: [job name, args..., NULL].
  /*char *jobArgs [job.args.size() + 2];
  jobArgs[0] = (char *) job.exec.c_str();
  for (int i = 1; i <= job.args.size(); ++i) {
    jobArgs[i] = (char *) job.args[i - 1].c_str();
  }
  // "The list of arguments must be terminated by a NULL pointer, and, since
  // these are variadic functions, this pointer must be cast (char *) NULL."
  jobArgs[job.args.size() + 1] = NULL;

  if (execvp(jobArgs[0], jobArgs) == ERROR_OCURRED) return false;
  else return true;*/

  /** COMMENTED **/
  /*closeFileDescriptor(descriptor[FIRST_PIPE_FD][STDIN_FILENO]);
  closeFileDescriptor(descriptor[FIRST_PIPE_FD][STDOUT_FILENO]);
  closeFileDescriptor(descriptor[SECOND_PIPE_FD][STDIN_FILENO]);
  closeFileDescriptor(descriptor[SECOND_PIPE_FD][STDOUT_FILENO]);
  printf("%s\n", job.exec.c_str());*/
  /** END COMMENTED **/
/*}*/

/*bool initializePipe(pipe_desc pipeToInit, vector <job_desc> &jobs) {
  int originalInput, originalOutput;
  if (pipeToInit.input != STD_IN) {
    originalInput = redirectStreamToFile(STDIN_FILENO, pipeToInit.input.c_str(),
                                         O_RDWR);
    if (originalInput == ERROR_OCURRED) return false;
  }
  if (pipeToInit.output != STD_OUT) {
    originalOutput = redirectStreamToFile(STDOUT_FILENO,
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
}*/

/**
  Connects the process with a pipe for input and output if possible, and
  executes the given job.
  @param descriptor File descriptors table, contains as many descriptors as
                    jobsCount - 1. Each descriptor has input and output slot.
  @param jobPosition Position of the job in the pipe sequence.
  @param jobsCount Total number of jobs in the pipe sequence.
  @param job Job to be executed.
  @return On success, returns true. On error, returns false and errno is set
          appropriately.
 */
bool executeProcessN(int descriptor[][2], int jobPosition, int jobsCount,
                     job_desc job) {
  // Take input from previous pipe if possible.
  if (jobPosition > 0) {
    closeFileDescriptor(descriptor[jobPosition - 1][STDOUT_FILENO]);
    if (!setupPipeDescriptor(descriptor[jobPosition - 1][STDIN_FILENO],
                             STDIN_FILENO)) return false;
  }

  // Write output to next pipe if possible.
  int pipesCount = jobsCount - 1;
  if (jobPosition != pipesCount) {
    closeFileDescriptor(descriptor[jobPosition][STDIN_FILENO]);
    if (!setupPipeDescriptor(descriptor[jobPosition][STDOUT_FILENO],
                             STDOUT_FILENO)) return false;
  }

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
}

/**
  Redirects the input and output streams for a given pipe if needed, does as
  many forks as number of jobs in the pipe, executing them handling required
  files descriptors and waiting until the last job in the pipe finishes it's
  execution.
  @param pipeToInit Description of the pipe to initialize.
  @param allJobs Reference to vector that contains all jobs (also those which
                 don't belong to the given pipe).
  @return true if initializing and running the pipe was successfully, false
          otherwise.
 */
bool initializePipeN(pipe_desc pipeToInit, vector <job_desc> &allJobs) {
  int originalInput, originalOutput;
  if (pipeToInit.input != STD_IN) {
    originalInput = redirectStreamToFile(STDIN_FILENO, pipeToInit.input.c_str(),
                                         O_RDWR);
    if (originalInput == ERROR_OCURRED) return false;
  }
  if (pipeToInit.output != STD_OUT) {
    originalOutput = redirectStreamToFile(STDOUT_FILENO,
                                          pipeToInit.output.c_str(),
                                          O_RDWR | O_CREAT);
    if (originalOutput == ERROR_OCURRED) return false;
  }
  // FORKS Y la vuelta aquí
  int jobsCount = pipeToInit.jobsIndexes.size();
  // If the pipe doesn't have any associated job so we're done.
  if (jobsCount == 0) return true;

  // Prepare n - 1 file descriptors, in each child we should close those that
  // are not needed.
  int pipesCount = jobsCount - 1;
  int descriptor[pipesCount][2];

  pid_t lastJob;
  for (int i = 0; i < jobsCount; ++i) {
    // If the current job is not the last one, create the pipe that follows it
    if (i != jobsCount - 1) {
      if (pipe(descriptor[i]) == ERROR_OCURRED) return false;
    }
    // We can close second previous pipe if it is valid because we won't use
    // it anymore.
    if (i - 2 >= 0) {
      closeFileDescriptor(descriptor[i - 2][STDIN_FILENO]);
      closeFileDescriptor(descriptor[i - 2][STDOUT_FILENO]);
    }
    pid_t currentChild;
    int jobIndex;
    job_desc currentJob;
    switch (currentChild = fork()) {
      case ERROR_OCURRED:
        return false;
      case 0:
        jobIndex = pipeToInit.jobsIndexes[i];
        currentJob = allJobs[jobIndex];
        if (!executeProcessN(descriptor, i, jobsCount, currentJob)) exit(errno);
        else exit(EXIT_SUCCESS);
        break;
      default:
        lastJob = currentChild;
        break;
    }
  }

  for (int i = 0; i < pipesCount; ++i) {
    if (isOpen(descriptor[i][STDIN_FILENO])) {
      closeFileDescriptor(descriptor[i][STDIN_FILENO]);
    }
    if (isOpen(descriptor[i][STDOUT_FILENO])) {
      closeFileDescriptor(descriptor[i][STDOUT_FILENO]);
    }
  }

  vector <pid_t> lastJobInVector(1, lastJob);
  return waitForChildren(lastJobInVector);
}

/**
  Receives a parent pipe process id and waits until it finishes it execution,
  then, checks for its exit status and prints a message according to it.
  @param exitedPipeId Parent pipe process id to wait and analyze.
  @param pipeName Name of the pipe to print in messages.
 */
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

/**
  Creates a parent pipe process forking and initializing it.
  @param pipeToCreate Description of the pipe to be created.
  @param allJobs Reference to vector that contains all jobs (also those which
                 don't belong to the given pipe).
 */
pid_t forkToCreatePipe(pipe_desc pipeToCreate, vector <job_desc> &allJobs) {
  pid_t child;
  printf("## Output %s ##\n", pipeToCreate.name.c_str());
  switch (child = fork()) {
    case ERROR_OCURRED:
      // An error ocurred while trying to fork.
      printResult(false, pipeToCreate.name, errno);
      break;
    case 0:
      //if (!initializePipe(pipeToCreate, jobs)) exit(errno);
      if (!initializePipeN(pipeToCreate, allJobs)) exit(errno);
      else exit(EXIT_SUCCESS);
    break;
  }
  return child;
}

int
main(int argc, char **argv) {
  // Contains all jobs data read and parsed from YAML file.
  vector <job_desc> jobs;
  // Contains all pipes data read and parsed from YAML file.
  vector <pipe_desc> pipes;
  // Contains all jobs indexes that were assigned to a pipe.
  set <int> assignedJobs;
  if (!checkArgs(argc)) return 0;
  // Loads data into jobs, pipes and assignedJobs from the YAML file specified
  // in arguments.
  if (!loadFile(jobs, pipes, argv[1], assignedJobs)) return 0;
  // Stores the process ids of each parent pipe processes.
  vector <pid_t> pipeCreators;
  for (int i = 0; i < pipes.size(); ++i) {
    // Execute current pipe.
    pid_t child = forkToCreatePipe(pipes[i], jobs);
    if (child > 0) pipeCreators.push_back(child);
  }
  for (int i = 0; i < pipeCreators.size(); ++i) {
    pipe_desc exitedPipe = pipes[i];
    pid_t exitedPipeId = pipeCreators[i];
    // Wait for current pipe to finish.
    analyzePipeResults(exitedPipeId, exitedPipe.name);
  }

  // Take all jobs that were not executed in any pipe and run them in a default
  // pipe.
  pipe_desc defaultPipe = buildDefaultPipe(jobs.size(), assignedJobs);
  if (!defaultPipe.jobsIndexes.empty()) {
    pid_t defaultPipeChild = forkToCreatePipe(defaultPipe, jobs);
    analyzePipeResults(defaultPipeChild, defaultPipe.name);
  }
  return 0;
}
