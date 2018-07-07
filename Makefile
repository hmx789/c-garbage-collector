elevator: elevator.c main.c 
	gcc -O0 -g elevator.c main.c -o elevator --std=gnu99

debug: elevator.c debug_main.c
	gcc -O0 -g elevator.c debug_main.c -o elevator --std=gnu99

clean: 
	rm elevator
