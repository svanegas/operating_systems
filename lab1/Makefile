FILENAME=jobRun
SRCPATH=./src/
BINPATH=./bin/
EXAMPLESPATH=./examples/
EXAMPLENAME=child

# Default is build
all: build

run: build
	$(BINPATH)$(FILENAME)

build: clean $(SRCPATH)$(FILENAME).cpp $(EXAMPLESPATH)$(EXAMPLENAME).cpp
	mkdir bin
	g++ $(SRCPATH)$(FILENAME).cpp -o $(BINPATH)$(FILENAME)
	g++ $(EXAMPLESPATH)$(EXAMPLENAME).cpp -o $(BINPATH)$(EXAMPLENAME)

clean:
	rm -rf ./bin
