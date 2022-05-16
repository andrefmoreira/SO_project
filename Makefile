mobile_node: mobile_node.c
        gcc -Wall mobile_node.c -o mobile_node

offload_simulator: offload_simulator.c
        gcc -Wall -pthread offload_simulator.c -o offload_simulator

all: offload_simulator mobile_node

clean:
        rm *.o all
