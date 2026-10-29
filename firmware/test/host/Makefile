# Host-Tests der hardwareunabhaengigen Logik fuer alle Bauformen:  make -C test/host
CXX ?= g++
CORE = ../../lib/prisma_core/src
SRC  = test_core.cpp $(CORE)/prisma_core.cpp
FLAGS = -std=c++17 -Wall -Wextra -O1 -I$(CORE)
BINS = test_muenze8 test_muenze12 test_prisma10

all: $(BINS)
	@for b in $(BINS); do echo "== $$b"; ./$$b || exit 1; done

test_muenze8: $(SRC) $(CORE)/prisma_core.h
	$(CXX) $(FLAGS) $(SRC) -o $@ -lm

test_muenze12: $(SRC) $(CORE)/prisma_core.h
	$(CXX) $(FLAGS) -DPRISMA_RING=12 $(SRC) -o $@ -lm

test_prisma10: $(SRC) $(CORE)/prisma_core.h
	$(CXX) $(FLAGS) -DPRISMA_PRISMA10 $(SRC) -o $@ -lm

clean:
	rm -f $(BINS) test_core

.PHONY: all clean
