collector: collector.c main.c 
	gcc -O0 -g collector.c main.c -o collector --std=gnu99

debug: collector.c debug_main.c
	gcc -O0 -g collector.c debug_main.c -o collector --std=gnu99

clean: 
	rm collector
