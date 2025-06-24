.PHONY: all old compile old-compile install remove clean

all: compile install

old: old-compile install
	
compile:
	gcc -O3 -march=native -s main.c -o defetch
	strip defetch

old-compile:
	gcc main.c -o defetch

install:
	sudo cp defetch /bin

remove:
	sudo rm -f /bin/defetch
	
clean:
	rm defetch
