#ifndef JOB_DESC_H
#define JOB_DESC_H

#include <string>
#include <vector>

extern const int PARAMS_COUNT;
extern const std::string JOBS_ATTR;
extern const std::string PIPES_ATTR;
extern const std::string PIPE_ATTR;
extern const std::string NAME_ATTR;
extern const std::string EXEC_ATTR;
extern const std::string ARGS_ATTR;
extern const std::string STD_IN;
extern const std::string STD_OUT;
extern const std::string STD_ERR;
extern const std::string WRITE_MODE;
extern const std::string READ_MODE;
extern const int FD_CLOSED;
extern const int FIRST_PIPE_FD;
extern const int SECOND_PIPE_FD;

struct job_desc {
  std::string name, exec;
  std::vector <std::string> args;
  //bool loadFromYAML(job_desc &destination, char* fileName);
};

struct pipe_desc {
  std::string name, input, output;
  std::vector <int> jobsIndexes;
};

bool loadFromYAML(std::vector <job_desc> &jobs, std::vector <pipe_desc> &pipes,
                  char* fileName);

#endif
