
#include <iostream>
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
#include <map>
#include <fstream>
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
  Receives a processes id and waits until it finishes.
  @param child Reference to the child process id to wait.
  @return true if child process exited successfully, false otherwise.
 */
bool waitForChild(pid_t child) {
  int status;
  if (waitpid(child, &status, 0) == child) {
    // WIFEXITED returns true if the child terminated normally.
    if (WIFEXITED(status)) {
      // WEXITSTATUS returns the exit status of the child.
      int code = WEXITSTATUS(status);
      if (code != EXIT_SUCCESS) {
        errno = code;
        return false;
      }
    }
    // WIFSIGNALED returns true if the child process was terminated by a signal.
    else if (WIFSIGNALED(status)) {
      // Returns the number of the signal that caused the child process to
      // terminate.
      int signalCode = WTERMSIG(status);
      errno = signalCode;
      return false;
    }
  }
  return true;
}

/**
  Takes a jobCount, set of assigned jobs then creates and fills a pipe
  description (pipe_desc) with the jobs that don't occur in the set.
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
    if (assignedJobs.count(i) == 0) defaultPipe.jobsIndexes.push_back(i);
  }
  return defaultPipe;
}

/**
  Creates all temporal files for each pipe. A single pipe will write it's
  output to a temporal file and the root process will wait for each process to
  finish, so that he can print the output for that pipe reading it from the
  temporal file. That is made to avoid a mixure of outputs in standard stream.
  @param pipes Reference to the vector of pipes, each pipe will contain the
               name of the temporal file in the 'tempOutput' attribute.
  @return true if temporal files were created successfully, false otherwise.
 */
bool createTemporalFiles(vector<pipe_desc> &pipes) {
  // Create the temporal directory with neccesary permissions.
  if (mkdir(TEMP_DIR.c_str(), S_IRWXU | S_IRWXG | S_IROTH | 
            S_IXOTH) == ERROR_OCURRED) return false;

  for (int i = 0; i < pipes.size(); ++i) {
    string temporalName = pipes[i].tempOutput;
    ofstream ofs(temporalName.c_str());
    ofs.close();
  }
  return true;
}

/**
  Deletes all temporal files that were used by each pipe to print it's output,
  also, it deletes the tmp folder.
  @param pipes Pipes that used a temporal file.
 */
void deleteTemporalFiles(vector<pipe_desc> &pipes) {
  for (int i = 0; i < pipes.size(); ++i) {
    string temporalName = pipes[i].tempOutput;
    remove(temporalName.c_str());
  }
  rmdir(TEMP_DIR.c_str());
}

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
bool executeProcess(int descriptor[][2], int jobPosition, int jobsCount,
                     job_desc job) {
  // Take input from previous pipe if possible.
  // The current process needs to read from the previous pipe, not to write on
  // it. That's why it closes the output slot and enables the input slot.
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
  files descriptors and waiting until the last job in the pipe finishes its
  execution.
  @param pipeToInit Description of the pipe to initialize.
  @param allJobs Reference to vector that contains all jobs (also those which
                 don't belong to the given pipe).
  @return true if initializing and running the pipe was successfully, false
          otherwise.
 */
bool initializePipe(pipe_desc pipeToInit, vector <job_desc> &allJobs) {
  int originalInput, originalOutput;

  // Redirect input if needed.
  if (pipeToInit.input != STD_IN) {
    originalInput = redirectStreamToFile(STDIN_FILENO, pipeToInit.input.c_str(),
                                         O_RDWR);
    if (originalInput == ERROR_OCURRED) return false;
  }

  // Always redirect output to temporal file.
  originalOutput = redirectStreamToFile(STDOUT_FILENO,
                                        pipeToInit.tempOutput.c_str(),
                                        O_RDWR | O_CREAT);
  if (originalOutput == ERROR_OCURRED) return false;

  // Prepare jobs to execute.
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
        if (!executeProcess(descriptor, i, jobsCount, currentJob)) exit(errno);
        else exit(EXIT_SUCCESS);
        break;
      default:
        lastJob = currentChild;
        break;
    }
  }

  // Ensure all file drescriptors are closed after all childs were executed.
  for (int i = 0; i < pipesCount; ++i) {
    if (isOpen(descriptor[i][STDIN_FILENO])) {
      closeFileDescriptor(descriptor[i][STDIN_FILENO]);
    }
    if (isOpen(descriptor[i][STDOUT_FILENO])) {
      closeFileDescriptor(descriptor[i][STDOUT_FILENO]);
    }
  }
  return waitForChild(lastJob);
}

/**
  Receives a pipe description which has already finished it's execution and
  prints the output that it generated in the temporal file.
  @param pipeToPrint Pipe to print it's output.
 */
void printPipeResults(pipe_desc pipeToPrint) {
  printf("## Output %s ##\n", pipeToPrint.name.c_str());
  string temporalName = pipeToPrint.tempOutput;
  // To read from temporal file, use an input file stream.
  ifstream ifs;
  ifs.open(temporalName.c_str());
  // To write to a file if needed, use a output file stream.
  ofstream ofs;
  if (pipeToPrint.output != STD_OUT) ofs.open(pipeToPrint.output.c_str());
  if (ifs.is_open()) {
    string line;
    // If we have to write in a file, do it.
    if (ofs.is_open()) while (getline(ifs, line)) ofs << line << endl;
    else while (getline(ifs, line)) printf("%s\n", line.c_str());
    ifs.close();
  }
  if (ofs.is_open()) ofs.close();
}

/**
  Checks for the received exit status and prints a message according to it.
  @param exitedPipeId Parent pipe process id to wait and analyze.
  @param pipeName Name of the pipe to print in messages.
 */
void analyzeExitStatus(int status, string pipeName) {
  // Returns true if the child terminated normally.
  if (WIFEXITED(status)) {
    // Returns the exit status of the child.
    int code = WEXITSTATUS(status);
    if (code != EXIT_SUCCESS) printResult(false, pipeName, code);
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

/**
  Creates a parent pipe process forking and initializing it.
  @param pipeToCreate Description of the pipe to be created.
  @param allJobs Reference to vector that contains all jobs (also those which
                 don't belong to the given pipe).
 */
pid_t forkAndCreatePipe(pipe_desc pipeToCreate, vector <job_desc> &allJobs) {
  pid_t child;
  switch (child = fork()) {
    case ERROR_OCURRED:
      // An error ocurred while trying to fork.
      return ERROR_OCURRED;
    case 0:
      if (!initializePipe(pipeToCreate, allJobs)) exit(errno);
      else exit(EXIT_SUCCESS);
    break;
  }
  return child;
}

int
main(int argc, char **argv) {
  if (!checkArgs(argc)) return 0;

  // Contains all jobs data read and parsed from YAML file.
  vector <job_desc> jobs;
  // Contains all pipes data read and parsed from YAML file.
  vector <pipe_desc> pipes;
  // Contains all jobs indexes that were assigned to a pipe.
  set <int> assignedJobs;
  // Loads data into jobs, pipes and assignedJobs from the YAML file specified
  // in arguments.
  if (!loadFile(jobs, pipes, argv[1], assignedJobs)) return 0;

  // Create temporal files that will be used by each pipe.
  createTemporalFiles(pipes);

  // Map from process id to pipe. Used to get the pipe that finished in the
  // wait function.
  map <pid_t, pipe_desc> pidToPipe;
  for (int i = 0; i < pipes.size(); ++i) {
    // Execute current pipe.
    pid_t child = forkAndCreatePipe(pipes[i], jobs);
    // Only if I'm the parent, add the process id to the map.
    if (child > 0) pidToPipe[child] = pipes[i];
  }

  // Take all jobs that were not executed in any pipe and run them in a default
  // pipe.
  pipe_desc defaultPipe = buildDefaultPipe(jobs.size(), assignedJobs);
  // If there is at least one process in the default pipe, go ahead and run it.
  if (!defaultPipe.jobsIndexes.empty()) {
    defaultPipe.tempOutput = TEMP_DIR + DEFAULT_PIPE + TEMP_EXT;
    pid_t defaultPipeChild = forkAndCreatePipe(defaultPipe, jobs);
    // Only if I'm the parent, add the process id to the map.
    if (defaultPipeChild > 0) pidToPipe[defaultPipeChild] = defaultPipe;
  }

  // Wait for each pipe and when it finishes print the corresponding output.
  int exitedPipes = 0;
  while (exitedPipes++ < pidToPipe.size()) {
    int status;
    // This wait will catch the first pipe-master that terminates its
    // execution.
    pid_t exitedPipeId = wait(&status);
    // If the wait didn't fail, proceed to show the results of the finished
    // process.
    if (exitedPipeId != ERROR_OCURRED) {
      pipe_desc pipeToPrint = pidToPipe[exitedPipeId];
      printPipeResults(pipeToPrint);
      analyzeExitStatus(status, pipeToPrint.name);
    }
  }

  // Add default pipe before calling delete temporal files, so that it can
  // find and delete the temporal file that default pipe used;
  pipes.push_back(defaultPipe);
  deleteTemporalFiles(pipes);
  return 0;
}
