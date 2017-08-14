INCLUDES    := -I/usr/local/include
LIBDIRS     := -L/usr/local/lib
LIBS        := -lgtest -lgmock -lpthread
CXXFLAGS    := -std=c++14 -g -Wall
CXX         := g++


.PHONY:    clean

default : all

all: main.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) main.cpp $(LIBDIRS) `wx-config --cxxflags --libs` -o test_chatter $(LIBS)

clean:
	rm -f test_chatter


#LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
