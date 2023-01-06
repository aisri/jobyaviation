.PHONY: all build run clean default

OPDIR=build
SIMDIR=sim

CPP=clang++
PROG=evtolsim
LIBS=jsoncpp
EVTOLDB=data/aircrafts.json

SRCS=$(wildcard src/*.cpp)
LINK=$(addprefix -L,$(LIBS))
INCS=$(addprefix -I,$(wildcard src/*.h))
OBJS=$(addprefix $(OPDIR)/, $(notdir $(patsubst %.cpp,%.o,$(SRCS))))

APPL=$(SIMDIR)/$(PROG)

$(info $(SRCS) $(OBJS) $(INCS) $(LINK))

$(OBJS):
	$(CPP) $(INCS) -o $@

build: $(OBJS)
	$(CPP) $(LINK) -o $(APPL)

run:
	$(APPL) $(EVTOLDB)

default: build

clean:
	rm -rf $(OPDIR)
	rm -rf $(SIMDIR)

all: clean build run
