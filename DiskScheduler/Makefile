CC=g++ -g -Wall -std=c++17

# List of source files for your disk scheduler
SCHEDULER_SOURCES=main_rewrite.cpp

# thread library object file
LIBOBJ=libthread.o

# current compilation system. CHANGE THIS WHEN TEST ON CAEN
CURSYS=mac

ifeq ($(CURSYS),mac)
	LIBOBJ=libthread_macos.o
	CC += -D_XOPEN_SOURCE
endif

# Generate the names of the disk scheduler's object files
SCHEDULER_OBJS=${SCHEDULER_SOURCES:.cpp=.o}

all: scheduler

# Compile the disk scheduler and tag this compilation
scheduler: ${SCHEDULER_OBJS} ${LIBOBJ}
	./autotag.sh
	${CC} -o $@ $^ -ldl -pthread

# Generic rules for compiling a source file to an object file
%.o: %.cpp
	${CC} -c $<
%.o: %.cc
	${CC} -c $<

clean:
	rm -f ${SCHEDULER_OBJS} scheduler
