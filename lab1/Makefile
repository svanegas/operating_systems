FILENAME=jobRun
HEADER=jobdesc
SRCPATH=./src/
BINPATH=./bin/
EXAMPLESPATH=./examples/
SAMPLE2SRC=2/sample2_src
SAMPLE1=1/sample1.yml
SAMPLE2=2/sample2.yml
YAMLFLAG=lyaml-cpp
CUSTOMPARSEFLAG=customparse

# Default is build
all: build

run: build buildsamples
	@$(BINPATH)$(FILENAME) $(EXAMPLESPATH)$(SAMPLE1)
	@$(BINPATH)$(FILENAME) $(EXAMPLESPATH)$(SAMPLE2) -$(CUSTOMPARSEFLAG)

build: clean $(SRCPATH)$(FILENAME).cpp $(SRCPATH)$(HEADER).cpp
	@mkdir $(BINPATH)
	@g++ $(SRCPATH)$(FILENAME).cpp $(SRCPATH)$(HEADER).cpp\
			 -o $(BINPATH)$(FILENAME) -$(YAMLFLAG)

buildsamples: cleansample2outerr $(EXAMPLESPATH)$(SAMPLE2SRC).cpp
	@mkdir -p $(BINPATH)/2
	@g++ $(EXAMPLESPATH)$(SAMPLE2SRC).cpp -o $(BINPATH)$(SAMPLE2SRC)

clean:
	@rm -rf $(BINPATH)/2
	@rm -rf $(BINPATH)

cleansample2outerr:
	@rm -rf $(EXAMPLESPATH)/2/out.txt $(EXAMPLESPATH)/2/err.txt
