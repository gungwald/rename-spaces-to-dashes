PROGRAM=replace-spaces-in-file-name

all: $(PROGRAM)

$(PROGRAM): $(PROGRAM).c
	gcc -Wall -O3 -o $(PROGRAM) $(PROGRAM).c

clean:
	rm $(PROGRAM)
