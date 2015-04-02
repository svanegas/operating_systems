#ifndef JOB_DESC_H
#define JOB_DESC_H

#include <string>
#include <vector>

extern const int PARAMS_COUNT;
extern const std::string NAME_ATTR;
extern const std::string EXEC_ATTR;
extern const std::string ARGS_ATTR;
extern const std::string INPUT_ATTR;
extern const std::string OUTPUT_ATTR;
extern const std::string ERROR_ATTR;

struct job_desc {
  std::string name, exec, input, output, error;
  std::vector <std::string> args;
  bool loadFromYAML(job_desc &destination, char* fileName);
};

#endif
