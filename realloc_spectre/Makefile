all: poc victim kernel_module/spectre_module.ko

victim: victim.c
	gcc victim.c -o victim

poc: attacker.c
	gcc attacker.c -o poc

kernel_module/spectre_module.ko:
	cd kernel_module && make

clean:
	cd kernel_module && make clean
	rm -f poc victim

run:
	taskset -c 0 sudo ./poc
