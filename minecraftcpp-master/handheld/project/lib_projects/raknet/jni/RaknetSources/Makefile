MODULE := raknet
SOURCES := $(wildcard *.cpp)

CFLAGS:=-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads $(CFLAGS)
CFLAGS+=-DNDEBUG -s -O1
LDFLAGS:=-L/opt/vc/lib -lpthread
CC:=g++

OBJECTS:=$(SOURCES:%.cpp=%.o)

all: $(MODULE)

%.o: %.cpp
	$(CC) -fpic -g -c $(CFLAGS) $< -o $@

$(MODULE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -shared -o libRakNet.so

static: $(OBJECTS)
	ar rvs libRaknet.a $(OBJECTS)

clean:
	rm -f $(OBJECTS) $(MODULE)
