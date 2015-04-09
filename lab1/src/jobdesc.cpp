#include <cstdio>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include "jobdesc.h"
#include "yaml-cpp/yaml.h"

using namespace std;

const int PARAMS_COUNT   = 6;

const string JOB_ATTR    = "Job";
const string NAME_ATTR   = "Name";
const string EXEC_ATTR   = "Exec";
const string ARGS_ATTR   = "Args";
const string INPUT_ATTR  = "Input";
const string OUTPUT_ATTR = "Output";
const string ERROR_ATTR  = "Error";
const string STD_IN      = "stdin";
const string STD_OUT     = "stdout";
const string STD_ERR     = "stderr";
const string WRITE_MODE  = "w";
const string READ_MODE   = "r";

/**
    Receives a reference to a string and trims it, that is, removes it's
    trailing and leading whitespaces or tabs.
    @param s Reference to string to be trimmed.
 */
void trim(string &s) {
  int i = 0;
  while (s[i] == ' ' || s[i] == '\t') ++i;
  s.erase(0, i);
  i = s.size() - 1;
  while (s[i] == ' ' || s[i] == '\t') --i;
  if (i != s.size() - 1) s.erase(i + 1, s.size());
}

/**
    Receives a reference to a string and removes it's surrounding quotes
    (if they exist).
    @param s Reference to string to be modified.
  */
void removeQuotes(string &s) {
  if (s.size() <= 1) return;
  if (s[0] == '\"' && s[s.size() - 1] == '\"') {
    s.erase(0, 1),
    s.erase(s.size() - 1, 1);
  }
}

/**
    Receives a reference to a string and removes it's surrounding brackets
    (if they exist).
    @param s Reference to string to be modified.
*/
void removeBrackets(string &s) {
  if (s.size() <= 1) return;
  if (s[0] == '[' && s[s.size() - 1] == ']') {
    s.erase(0, 1),
    s.erase(s.size() - 1, 1);
  }
}

/**
    This method is used for the custom parser in order to read a line from
    a input file stream and parse it's key and value, where the key is the
    attribute of the job.
    @param ifs Reference to ifstream that contains the stream with the job
               YAML description.
    @param value Reference to string where the value will be stored.
    @return A string with the name of the attribute (key).
  */
string parseAttribute(ifstream &ifs, string &value) {
  string key;
  // Ignore <Key> :
  ifs >> key >> value;
  // Get the remaining line (value).
  getline(ifs, value);
  // Trim, remove quoutes and remove brackets to the resulting string.
  trim(value);
  removeQuotes(value);
  removeBrackets(value);
  return key;
}

/**
    Method that uses a custom implementation to parse a YAML file and fill
    a job_desc with the respective values.
    @param destination Reference to the job_desc (job description) to be filled.
    @param fileName Name of the YAML file to be opened and parsed.
    @return true if the file was successfully parsed, false otherwise.
  */
bool parseFromCustom(job_desc &destination, char* fileName) {
  // Bitmask used to determine whether all attributes were parsed
  // successfully or not. Each attribute is a bit, starting from Name (0) to
  // Error (5). If all attributes were parsed the mask should look like this:
  // 111111, that is equal to (1 << 6) - 1.
  short completeMask = 0;
  ifstream ifs (fileName, ifstream::in);
  if (!ifs.is_open()) return false;
  string buffer;
  ifs >> buffer >> buffer >> buffer >> buffer; // - Job : -

  for (int i = 0; i < PARAMS_COUNT; ++i) {
    string attrName = parseAttribute(ifs, buffer);
    if (attrName == NAME_ATTR) {
      destination.name = buffer;
      completeMask |= (1 << 0);
    }
    else if (attrName == EXEC_ATTR) {
      destination.exec = buffer;
      completeMask |= (1 << 1);
    }
    else if (attrName == ARGS_ATTR) {
      // Use a stringstream to split arguments by comma (,)
      stringstream splitter(buffer);
      while (getline(splitter, buffer, ',')) {
        // We should trim and remove quotes to each single argument
        trim(buffer);
        removeQuotes(buffer);
        destination.args.push_back(buffer);
      }
      completeMask |= (1 << 2);
    }
    else if (attrName == INPUT_ATTR) {
      destination.input = buffer;
      completeMask |= (1 << 3);
    }
    else if (attrName == OUTPUT_ATTR) {
      destination.output = buffer;
      completeMask |= (1 << 4);
    }
    else if (attrName == ERROR_ATTR) {
      destination.error = buffer;
      completeMask |= (1 << 5);
    }
  }
  // So, as explained above, if all attributes were read we return true, false
  // otherwise.
  return completeMask == (1 << PARAMS_COUNT) - 1;
}

/**
    Method that uses 'yaml-cpp' library to parse a YAML file and fill a
    job_desc with the respective values.
    @param destination Reference to the job_desc (job description) to be filled.
    @param fileName Name of the YAML file to be opened and parsed.
    @return true if the file was successfully parsed, false otherwise.
*/
bool parseFromLib(job_desc &destination, char* fileName) {
  // Bitmask used to determine whether all attributes were parsed
  // successfully or not. Each attribute is a bit, starting from Name (0) to
  // Error (5). If all attributes were parsed the mask should look like this:
  // 111111, that is equal to (1 << 6) - 1.
  short completeMask = 0;
  YAML::Node node = YAML::LoadFile(fileName);
  // If the 'Job' node doesn't exist return false because we couldn't load
  // YAML file.
  if (!node[0][JOB_ATTR][0]) return false;
  node = node[0][JOB_ATTR][0]; // Map inside Job
  // Iterate through pairs found in Job node.
  for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
    // Get the key, i.e. attribute name as string.
    string attrName = (it->first).as<string>();
    // Save the value in other node.
    YAML::Node attrValue = it->second;
    if (attrName == NAME_ATTR) {
      destination.name = attrValue.as<string>();
      completeMask |= (1 << 0);
    }
    else if (attrName == EXEC_ATTR) {
      destination.exec = attrValue.as<string>();
      completeMask |= (1 << 1);
    }
    else if (attrName == ARGS_ATTR) {
      // Iterate through the arguments and push them in the vector.
      for (int i = 0; i < attrValue.size(); ++i) {
        destination.args.push_back(attrValue[i].as<string>());
      }
      completeMask |= (1 << 2);
    }
    else if (attrName == INPUT_ATTR) {
      destination.input = (it->second).as<string>();
      completeMask |= (1 << 3);
    }
    else if (attrName == OUTPUT_ATTR) {
      destination.output = (it->second).as<string>();
      completeMask |= (1 << 4);
    }
    else if (attrName == ERROR_ATTR) {
      destination.error = (it->second).as<string>();
      completeMask |= (1 << 5);
    }
  }
  // So, as explained above, if all attributes were read we return true, false
  // otherwise.
  return completeMask == (1 << PARAMS_COUNT) - 1;
}

/**
    Loads a job description from a YAML file, storing all the data in the
    job_desc structure given.
    @param description Reference to job_desc where data will be saved.
    @param fileName Name of the YAML file that contains the job description.
    @param parseMode If 1 'yaml-cpp' library will be used to parse, if 2 the
                     custom parse implementation will be used.
  */
bool job_desc::loadFromYAML(job_desc &destination, char* fileName,
                            int parseMode) {
  bool parseResult;
  switch (parseMode) {
    case LIB_PARSE:
      parseResult = parseFromLib(destination, fileName);
      break;
    case CUSTOM_PARSE:
      parseResult = parseFromCustom(destination, fileName);
      break;
    default:
      parseResult = parseFromLib(destination, fileName);
      break;
  }
  return parseResult;
}
