PROGRAM=replace-spaces-in-file-name
INSTALL_PREFIX=/usr/local

all: $(PROGRAM)

$(PROGRAM): $(PROGRAM).c
	gcc -g -Wall -O3 -o $(PROGRAM) $(PROGRAM).c

install: $(PROGRAM)
	install --strip $(PROGRAM) $(INSTALL_PREFIX)/bin

uninstall: $(PROGRAM)
	rm $(INSTALL_PREFIX)/bin/$(PROGRAM) 

clean:
	rm $(PROGRAM)

# TODO - Create a man page, add install & uninstall of man page here
