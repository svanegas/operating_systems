FILENAME=jobRun
HEADER=jobdesc
SRCPATH=./src/
BINPATH=./bin/
EXAMPLESPATH=./examples/
EXAMPLENAME=sample2_src
SAMPLE1=sample1.yml
SAMPLE2=sample2.yml

# Default is build
all: build

run: build buildsamples
	@$(BINPATH)$(FILENAME) $(EXAMPLESPATH)$(SAMPLE1)
	@$(BINPATH)$(FILENAME) $(EXAMPLESPATH)$(SAMPLE2)

build: clean $(SRCPATH)$(FILENAME).cpp $(SRCPATH)$(HEADER).cpp
	@mkdir bin
	@g++ $(SRCPATH)$(FILENAME).cpp $(SRCPATH)$(HEADER).cpp -o $(BINPATH)$(FILENAME)

buildsamples: $(EXAMPLESPATH)$(EXAMPLENAME).cpp
	@g++ $(EXAMPLESPATH)$(EXAMPLENAME).cpp -o $(BINPATH)$(EXAMPLENAME)

clean:
	@rm -rf ./bin
