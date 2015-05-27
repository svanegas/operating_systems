#include <cstdio>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <set>
#include "jobdesc.h"
#include "yaml-cpp/yaml.h"

using namespace std;

const int PARAMS_COUNT   = 6;

const string JOBS_ATTR    = "Jobs";
const string PIPES_ATTR   = "Pipes";
const string PIPE_ATTR    = "Pipe";
const string NAME_ATTR    = "Name";
const string EXEC_ATTR    = "Exec";
const string ARGS_ATTR    = "Args";
const string INPUT_ATTR   = "input";
const string OUTPUT_ATTR  = "output";
const string STD_IN       = "stdin";
const string STD_OUT      = "stdout";
const string WRITE_MODE   = "w";
const string READ_MODE    = "r";
const string DEFAULT_PIPE = "default-pipe";
const int FD_CLOSED       = -1;
const int FIRST_PIPE_FD   = 0;
const int SECOND_PIPE_FD  = 1;

/**
  Method that uses 'yaml-cpp' library to parse a YAML file and fill a vector of
  job_desc with the respective values.
  @param jobs Reference to the job_desc (job description) vector to be filled.
  @param rootNode Source YAML data loaded and represented as root node.
  @param jobIndexByName Map to be filled by job name as key and index in the
                        jobs vector as value.
  @return true if the file was successfully parsed, false otherwise.
*/
bool parseJobs(vector <job_desc> &jobs, YAML::Node &rootNode,
               map <string, int> &jobIndexByName) {
  YAML::Node jobsNode;
  if (!(jobsNode = rootNode[JOBS_ATTR])) return false;
  for (YAML::const_iterator jobsIt = jobsNode.begin(); jobsIt != jobsNode.end();
       ++jobsIt) {
    job_desc currentJob;
    YAML::Node currentJobNode = *jobsIt;
    // If required attribute doesn't exist return false.
    if (!currentJobNode[NAME_ATTR]) return false;
    currentJob.name = currentJobNode[NAME_ATTR].as<string>();
    if (!currentJobNode[EXEC_ATTR]) return false;
    currentJob.exec = currentJobNode[EXEC_ATTR].as<string>();
    if (!currentJobNode[ARGS_ATTR]) return false;
    YAML::Node argsNode = currentJobNode[ARGS_ATTR];
    for (int i = 0; i < argsNode.size(); ++i) {
      currentJob.args.push_back(argsNode[i].as<string>());
    }
    jobs.push_back(currentJob);
    // Set the index where we can find the job by it's name in a map.
    jobIndexByName[currentJob.name] = jobs.size() - 1;
  }
  // All jobs could be retrieved, so return true.
  return true;
}

bool parsePipes(vector <pipe_desc> &pipes, YAML::Node &rootNode,
                map <string, int> &jobIndexByName, set <int> &assignedJobs) {
  YAML::Node pipesNode;
  if (!(pipesNode = rootNode[PIPES_ATTR])) return false;
  for (YAML::const_iterator pipesIt = pipesNode.begin();
       pipesIt != pipesNode.end(); ++pipesIt) {
    pipe_desc currentPipe;
    YAML::Node currentPipeNode = *pipesIt;
    // If required attribute doesn't exist return false.
    if (!currentPipeNode[NAME_ATTR]) return false;
    currentPipe.name = currentPipeNode[NAME_ATTR].as<string>();
    if (!currentPipeNode[INPUT_ATTR]) return false;
    currentPipe.input = currentPipeNode[INPUT_ATTR].as<string>();
    if (!currentPipeNode[OUTPUT_ATTR]) return false;
    currentPipe.output = currentPipeNode[OUTPUT_ATTR].as<string>();
    if (!currentPipeNode[PIPE_ATTR]) return false;
    YAML::Node pipeNode = currentPipeNode[PIPE_ATTR];
    for (int i = 0; i < pipeNode.size(); ++i) {
      string jobName = pipeNode[i].as<string>();
      // Get the index of the job in the map
      int jobIndex = jobIndexByName[jobName];
      currentPipe.jobsIndexes.push_back(jobIndex);
      assignedJobs.insert(jobIndex);
    }
    pipes.push_back(currentPipe);
  }
  // All jobs could be retrieved, so return true.
  return true;
}

bool loadFromYAML(vector <job_desc> &jobs, vector <pipe_desc> &pipes,
                  char* fileName, set <int> &assignedJobs) {
  YAML::Node rootNode = YAML::LoadFile(fileName);
  map <string, int> jobIndexByName;
  if (!parseJobs(jobs, rootNode, jobIndexByName)) return false;
  if (!parsePipes(pipes, rootNode, jobIndexByName, assignedJobs)) return false;
  return true;
}
