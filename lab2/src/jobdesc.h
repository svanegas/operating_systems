#ifndef JOB_DESC_H
#define JOB_DESC_H

#include <string>
#include <vector>
#include <set>

extern const std::string JOBS_ATTR;
extern const std::string PIPES_ATTR;
extern const std::string PIPE_ATTR;
extern const std::string NAME_ATTR;
extern const std::string EXEC_ATTR;
extern const std::string ARGS_ATTR;
extern const std::string STD_IN;
extern const std::string STD_OUT;
extern const std::string STD_ERR;
extern const std::string DEFAULT_PIPE;
extern const std::string TEMP_DIR;
extern const std::string TEMP_EXT;
extern const int FD_CLOSED;

/**
  This structure stores the information of a job.
  */
struct job_desc {
  std::string name, exec;
  std::vector <std::string> args;
};

/**
  This structure stores the information of a pipe.
  */
struct pipe_desc {
  std::string name, input, output, tempOutput;
  std::vector <int> jobsIndexes;
};

/**
  Loads a list of jobs and another of pipes from a YAML file, storing all the
  data in the jobs and pipes vectors given. Also, a set of assignedJobs will
  be filled in order to know which jobs are assiged to a pipe.
  @param jobs Reference to a vector of job_desc where jobs data will be saved.
  @param pipes Reference to a vector of pipe_desc where pipes data will be
               saved.
  @param fileName Name of the YAML file that contains the jobs and pipes
                  description.
  @param assignedJobs Set of integers that represent jobs indexes that were
                      assigned to a pipe.
*/
bool loadFromYAML(std::vector <job_desc> &jobs, std::vector <pipe_desc> &pipes,
                  char* fileName, std::set<int> &assignedJobs);

/**
  Utility to convert an integer into a string.
  @param x Integer value to convert into string.
  @return String representation of x.
 */
std::string toStr(int x);
#endif
