.PHONY: all build run clean default

SIMDIR=sim

CPP=clang++
PROG=evtolsim
LIBS=jsoncpp

SIMCONF=data/simulation.json

SRCS=$(wildcard src/*.cpp)
LINK=$(addprefix -l,$(LIBS))
INCS=$(addprefix -I,$(wildcard src/*.h))

OPTS=-std=c++20 -Wall -Wextra

APPL=$(SIMDIR)/$(PROG)

# $(info $(SRCS) $(OBJS) $(INCS) $(LINK))

build:
	mkdir -p $(SIMDIR)
	$(CPP) $(SRCS) $(OPTS) $(LINK) -o $(APPL)

run:
	$(APPL) $(SIMCONF)

default: build

clean:
	rm -rf $(SIMDIR)

all: clean build run
