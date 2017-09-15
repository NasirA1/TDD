INCLUDES    := -I/usr/local/include -I./Chatter -I./ChatterTests
LIBDIRS     := -L/usr/local/lib
LIBS        := -lpthread
CXXFLAGS    := -std=c++14 -g -Wall
CXX         := g++


.PHONY:    clean

default : all

all: ChatterTests/main.cpp Chatter/wxChatterApp.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) ChatterTests/main.cpp $(LIBDIRS) -o chatter_tests $(LIBS) -lgtest -lgmock 
	$(CXX) $(CXXFLAGS) $(INCLUDES) Chatter/wxChatterApp.cpp $(LIBDIRS) `wx-config --cxxflags --libs` -o chatter_app $(LIBS)

clean:
	rm -f chatter_tests chatter_app


#LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
