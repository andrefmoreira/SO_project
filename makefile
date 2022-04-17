system_manager: system_manager.c
	gcc -Wall -pthread system_manager.c -o system_manager

mobile_node: mobile_node.c
	gcc -Wall mobile_node.c -o mobile_node

all: system_manager mobile_node

clean:
	rm *.o all