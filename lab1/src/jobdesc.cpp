#include <cstdio>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include "jobdesc.h"

using namespace std;

const int PARAMS_COUNT   = 6;

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

void trim(string &s) {
  int i = 0;
  while (s[i] == ' ' || s[i] == '\t') ++i;
  s.erase(0, i);
  i = s.size() - 1;
  while (s[i] == ' ' || s[i] == '\t') --i;
  if (i != s.size() - 1) s.erase(i + 1, s.size());
}

void removeQuotes(string &s) {
  if (s.size() <= 1) return;
  if (s[0] == '\"' && s[s.size() - 1] == '\"') {
    s.erase(0, 1),
    s.erase(s.size() - 1, 1);
  }
}

void removeBrackets(string &s) {
  if (s.size() <= 1) return;
  if (s[0] == '[' && s[s.size() - 1] == ']') {
    s.erase(0, 1),
    s.erase(s.size() - 1, 1);
  }
}

string parseAttribute(ifstream &ifs, string &value) {
  string key;
  ifs >> key >> value; // <Key> :
  getline(ifs, value);
  trim(value);
  removeQuotes(value);
  removeBrackets(value);
  return key;
}

void parseYAML(job_desc &destination, ifstream &ifs) {
  string buffer;
  ifs >> buffer >> buffer >> buffer >> buffer; // - Job : -

  for (int i = 0; i < PARAMS_COUNT; ++i) {
    string attrName = parseAttribute(ifs, buffer);
    if (attrName == NAME_ATTR) destination.name = buffer;
    else if (attrName == EXEC_ATTR) destination.exec = buffer;
    else if (attrName == ARGS_ATTR) {
      stringstream splitter(buffer);
      while (getline(splitter, buffer, ',')) {
        trim(buffer);
        removeQuotes(buffer);
        destination.args.push_back(buffer);
      }
    }
    else if (attrName == INPUT_ATTR) destination.input = buffer;
    else if (attrName == OUTPUT_ATTR) destination.output = buffer;
    else if (attrName == ERROR_ATTR) destination.error = buffer;
  }
}

bool job_desc::loadFromYAML(job_desc &destination, char* fileName) {
  ifstream ifs (fileName, ifstream::in);
  if (!ifs.is_open()) return false;
  parseYAML(destination, ifs);
  return true;
}
