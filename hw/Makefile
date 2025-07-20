
ifeq ($(SYSTEMC_PATH),)
    $(error Please configure your SystemC path)
endif

CXXFLAGS = -c -g -I$(SYSTEMC_PATH)/include
CFLAGS = -c -g -I$(SYSTEMC_PATH)/include -I..

CPP_SRCS := \
	master.cpp \
	slave.cpp \
	glue.cpp

C_SRCS := \
	main.c

OBJS := $(patsubst %.cpp,%.o,$(CPP_SRCS)) $(patsubst %.c,%.o,$(C_SRCS))

all: $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	$(RM) $(OBJS)
