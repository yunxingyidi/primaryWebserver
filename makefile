CXX ?= g++

DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
else
    CXXFLAGS += -O2

endif

server: main.cpp  ./time/timer.cpp ./http/http_handle.cpp ./log/log.cpp server.cpp config.cpp
	$(CXX) -o server  $^ $(CXXFLAGS) -lpthread 

clean:
	rm  -r server
