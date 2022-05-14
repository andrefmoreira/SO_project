CC = gcc
FLAGS = -Wall
LIBS = -pthread
FILE = log_file.txt

OBJS1 = system_manager.o task_manager.o monitor.o maintenance_manager.o file.o aux_funcs.o
OBJS2 = mobile_node.o

PROG1 = offload_simulator
PROG2 = mobile_node

all: ${PROG1} ${PROG2}


${PROG1}: ${OBJS1}
		${CC} ${FLAGS} ${LIBS} ${OBJS1} -o $@


${PROG2}: ${OBJS2}
		${CC} ${FLAGS} ${OBJS2} -o $@

.c.o:
		${CC} ${FLAGS} $< -c


system_manager.o: aux_funcs.c aux_funcs.h file.c file.h maintenance_manager.c maintenance_manager.h monitor.c monitor.h shared_memory.h


task_manager.o: task_manager.c task_manager.h shared_memory.h file.c file.h aux_funcs.c aux_funcs.h


monitor.o: monitor.c monitor.h


maintenance_manager.o: maintenance_manager.c maintenance_manager.h


file.o: file.c file.h


aux_funcs.o: aux_funcs.c aux_funcs.h



