INCLUDES    := -I/usr/local/include -I./Chatter -I./ChatterTests
LIBDIRS     := -L/usr/local/lib
LIBS        := -lpthread
CXXFLAGS    := -std=c++14 -g -Wall --coverage
CXX         := g++

COV_DIR 		:= $(shell pwd)/coverage
COV_STRIP		:= $(words $(subst /, ,$(COV_DIR)))


.PHONY:    	clean coverage


default: 		all


all: ChatterTests/main.cpp Chatter/wxChatterApp.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) ChatterTests/main.cpp $(LIBDIRS) -o chatter_tests $(LIBS) -lgtest -lgmock 
	$(CXX) $(CXXFLAGS) $(INCLUDES) Chatter/wxChatterApp.cpp $(LIBDIRS) `wx-config --cxxflags --libs` -o chatter_app $(LIBS)


coverage: chatter_tests
	mkdir -p coverage
	export GCOV_PREFIX=$(COV_DIR)
	export GCOV_PREFIX_STRIP=$(COV_STRIP)
	./chatter_tests
	sleep 1s
	lcov -t "result" -o coverage/ex_test.info -c -d .
	genhtml  -o ./coverage/res ./coverage/ex_test.info


clean:
	rm -rf chatter_tests chatter_app coverage/* *.gcno *.gcda

