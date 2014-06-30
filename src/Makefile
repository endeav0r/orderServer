OBJS = orderStore.o emdrListener.o evePwnServer.o

CXXFLAGS=-O2 -g -Werror -Wall -std=c++11
INCLUDE=-lmsgpack -ljansson -lzmq -lz -lpthread

all : orderServer

orderServer : $(OBJS) main.cc
	$(CXX) -o orderServer main.cc $(OBJS) $(INCLUDE) $(CXXFLAGS)

%.o : %.cc %.h
	$(CXX) -c -o $@ $< $(INCLUDE) $(CXXFLAGS)

%.o : %.c %.h
	$(CXX) -c -o $@ $< $(INCLUDE) $(CXXFLAGS)

clean :
	rm -f *.o
	rm -f orderServer