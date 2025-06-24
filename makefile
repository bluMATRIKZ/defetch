.PHONY: all old compile old-compile install remove clean

all: compile install

old: old-compile install
	
compile:
	gcc -O3 -march=native -s main.c -o litefetch
	strip litefetch

old-compile:
	gcc main.c -o litefetch

install:
	sudo cp litefetch /bin

remove:
	sudo rm -f /bin/litefetch
	
clean:
	rm litefetch
