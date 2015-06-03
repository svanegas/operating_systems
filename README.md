##### Operating Systems
---
# Programming Lab 2
by: [Alejandro Sánchez Aristizábal] & [Santiago Vanegas Gil]

## Overview

The *runPipe* program receives a file with jobs and pipes descriptions
(with certain options) as input and runs it.

The jobs and pipes description are given in a [YAML] file with the following
structure:

```sh
Jobs :
  - Name : <Job Name>
    Exec : <Executable>
    Args : [<Arguments>]
Pipes :
  - Name : <Pipe Name>
    Pipe : [<Jobs>]
    input : <Input>
    output : <Output>
```

The options that the job should have are:

- **Job Name:** is a descriptive name for the job.
- **Executable:** is the name of the program that will be run, either with an
absolute or relative path.
- **Arguments:** is a list of arguments for the program.

The options that the pipes should have are:

- **Pipe Name:** is a descriptive name for the pipe.
- **Jobs:** The list of jobs in that pipe. Note that the order in the list is
the actual redirection order.
- **Input:** whether to read from standard input (stdin) or from a file.
- **Output:** whether to write to standard output (stdout) or to a file.

## Try it yourself
The program uses [yaml-cpp] library to parse the YAML file. In order to compile
the project with this library it must be installed in your machine, you can
follow the steps given in [the official site] or follow the provided here.

### yaml-cpp installation
- Make sure you have CMake installed, if you don't, install it with:
> **Ubuntu:** `$ sudo apt-get install cmake`  
> **Red Hat/Fedora:** `$ sudo yum install cmake`

- In order to install *yaml-cpp* library you must have *libboost-all-dev*
package installed too, so execute:
> **Ubuntu:** `$ sudo apt-get install libboost-all-dev`  
> **Red Hat/Fedora:** `$ sudo yum install libboost-all-dev`

- Clone the *[yaml-cpp]* repository or download the [source package] and
extract it.
(**Note:** The *yaml-cpp* version used in this project was **0.5.1**)
```sh
$ git clone https://github.com/jbeder/yaml-cpp.git
```
- Step into the extracted directory or the cloned repository.
```sh
$ cd yaml-cpp
```
- Create a *build* directory and step into it.
```sh
$ mkdir build
$ cd build
```
- Run the following commands to prepare and finish the installation of the
library.
```sh
$ cmake ..
$ make
$ sudo make install
```

Now you can compile and run the program as follows.

### Compiling the project
Step into the *lab2* directory and compile the project using the *Makefile*
```sh
$ make build
```
### Running the project
After compiled, you can run some examples with:
```sh
$ make run
```
Or run the program with your favourite job descriptions following this command:
```sh
$ ./bin/runPipe <yaml-file>
```
The command above will run the program, take the specified YAML file as
input and execute the jobs using the pipes described on it.

### Example
Given this YAML file saved in the current working directory as
__*sample1.yml*__:
```sh
Jobs :
  - Name : "job1"
    Exec : "echo"
    Args : ["you", "gotta","be","kidding"]
  - Name : "job2"
    Exec : "tr"
    Args : [" ","_"]
  - Name : "job3"
    Exec : "yes"
    Args : ["No","I’m", "not!"]
  - Name : "job4"
    Exec : "head"
    Args : []
Pipes :
  - Name : "pipe1"
    Pipe : [ "job1","job2" ]
    input : "stdin"
    output : "stdout"
```
, and running the command:
```sh
$ ./bin/runPipe sample1.yml
```
the expected output is:
```sh
## Output pipe1 ##
you-gotta-be-kidding
## pipe1 finished successfully ##
## Output default-pipe ##
no I’m not!
no I’m not!
no I’m not!
no I’m not!
no I’m not!
no I’m not!
no I’m not!
no I’m not!
no I’m not!
no I’m not!
## default-pipe finished successfully ##
```
That is equivalent to (The order is irrelevant):
```sh
$ echo you gotta be kidding | tr " " _ &
$ yes No "I’m" "not!" | head &
```
[Alejandro Sánchez Aristizábal]:https://github.com/ibalejandro
[Santiago Vanegas Gil]:https://github.com/svanegas
[YAML]:http://www.yaml.org/spec/1.2/spec.html
[yaml-cpp]:https://github.com/jbeder/yaml-cpp
[the official site]:https://github.com/jbeder/yaml-cpp
[source package]:https://code.google.com/p/yaml-cpp/downloads/list
