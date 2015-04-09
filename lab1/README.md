##### Operating Systems
---
# Programming Lab 1
by: [Alejandro Sánchez Aristizábal] & [Santiago Vanegas Gil]

## Overview

The *jobRun* program receives a file with a job description (with certain
  options) as input and runs it.
The options that the job should have are:

- **Name:** is a descriptive name for the job.
- **Executable:** is the name of the program that will be run, either with an
absolute or relative path.
- **Arguments:** is a list of arguments for the program.
- **Input:** whether to read from standard input (stdin) or from a file.
- **Output:** whether to write to standard output (stdout) or to a file.
- **Error:** whether to write errors to standard error (stderr) or to a file.

The job description is given in a [YAML] file with the following structure

```sh
- Job :
  - Name : <Job Name>
      Exec : <Executable>
      Args : [<Arguments>]
      Input : <Input>
      Output : <Output>
      Error : <Error>
```

## Try it yourself
The program can be run in two ways, with a library parser or with a custom
parser. The library used in this case is [yaml-cpp]. In order to compile the
project with this library it must be installed in your machine, you can follow
the steps given in [the official site] or follow the provided here.

### yaml-cpp installation
1. Make sure you have CMake installed, if you don't, install it with:
```sh
$ sudo apt-get install cmake
```
2. In order to install *yaml-cpp* library you must have *libboost-all-dev*
package installed too, so execute:
```sh
$ sudo apt-get install libboost-all-dev
```
3. Clone the *[yaml-cpp]* repository or download the [source package] and
extract it. (**Note:** The *yaml-cpp* version used in this project was 0.5.1)
```sh
$ git clone https://github.com/jbeder/yaml-cpp.git
```
4. Step into the extracted directory or the cloned repository.
```sh
$ cd yaml-cpp
```
5. Create a *build* directory and step into it.
```sh
$ mkdir build
$ cd build
```
6. Run the following commands to prepare and finish the installation of the
library.
```sh
$ cmake ..
$ make
$ sudo make install
```

Now you can compile and run the program as follows.

### Compiling the project
Step into the *lab1* directory and compile the project using the *Makefile*
```sh
$ make build
```
### Running the project
After compiled, you can rum some examples with:
```sh
$ make run
```
Or run the program with your favourite job descriptions following this command:
```sh
$ ./bin/jobRun <yaml-file>
```
The command above will parse the given YAML file with the *yaml-cpp* library
by default. If you want to use the custom parser you can use the flag
_**customparse**_ as follows:
```sh
$ ./bin/jobRun <yaml-file> -customparse
```

### Example
Given this YAML file saved in the current working directory as
__*sample1.yml*__:
```sh
- Job :
  - Name : "test-job"
      Exec : "echo"
      Args : ["-n","this is a test"]
      Input : "stdin"
      Output : "stdout"
      Error : "stderr"

```
, and running the command:
```sh
$ ./bin/jobRun sample1.yml
```
the expected output is:
```sh
## Running test-job ##
this is a test
## test-job fished succesfully ##
```
[Alejandro Sánchez Aristizábal]:https://github.com/ibalejandro
[Santiago Vanegas Gil]:https://github.com/svanegas
[YAML]:http://www.yaml.org/spec/1.2/spec.html
[yaml-cpp]:https://github.com/jbeder/yaml-cpp
[the official site]:https://github.com/jbeder/yaml-cpp
[source package]:https://code.google.com/p/yaml-cpp/downloads/list
