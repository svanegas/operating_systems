FILENAME=runPipe
HEADER=jobdesc
SRCPATH=./src/
BINPATH=./bin/
EXAMPLESPATH=./examples/
SAMPLE2SRC=2/sample2_src
SAMPLE2DELAY=2/delay2_src
SAMPLE1=1/sample1.yml
SAMPLE2=2/sample2.yml
YAMLFLAG=lyaml-cpp

# Default is build
all: build

run: build buildsamples
	@$(BINPATH)$(FILENAME) $(EXAMPLESPATH)$(SAMPLE1)
	@$(BINPATH)$(FILENAME) $(EXAMPLESPATH)$(SAMPLE2)

build: clean $(SRCPATH)$(FILENAME).cpp $(SRCPATH)$(HEADER).cpp
	@mkdir $(BINPATH)
	@g++ $(SRCPATH)$(FILENAME).cpp $(SRCPATH)$(HEADER).cpp\
			 -o $(BINPATH)$(FILENAME) -$(YAMLFLAG)

buildsamples: cleansample2out $(EXAMPLESPATH)$(SAMPLE2SRC).cpp
	@mkdir -p $(BINPATH)/2
	@g++ $(EXAMPLESPATH)$(SAMPLE2SRC).cpp -o $(BINPATH)$(SAMPLE2SRC)
	@g++ $(EXAMPLESPATH)$(SAMPLE2DELAY).cpp -o $(BINPATH)$(SAMPLE2DELAY)

clean:
	@rm -rf $(BINPATH)/2
	@rm -rf $(BINPATH)

cleansample2out:
	@rm -rf $(EXAMPLESPATH)/2/out.txt
