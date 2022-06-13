COMPILER         = -c++
OPTIMIZATION_OPT = -O3
OPTIONS          = -pedantic -ansi -Wall -Werror $(OPTIMIZATION_OPT) -o
PTHREAD          = -lpthread
LINKER_OPT       = -L/usr/lib -std=c++1y $(PTHREAD) -lboost_thread -lboost_system

BUILD_LIST+=dispatcher_server

all: $(BUILD_LIST)

dispatcher_server: dispatcher_server.cpp
	$(COMPILER) $(OPTIONS) dispatcher_server dispatcher_server.cpp $(LINKER_OPT)

strip_bin :
	strip -s dispatcher

clean:
	rm -f core *.o *.bak *~ *stackdump *#
