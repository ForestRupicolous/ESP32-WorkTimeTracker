# Host-Tests der hardwareunabhaengigen Logik:  make -C test/host
CXX ?= g++
CORE = ../../lib/prisma_core/src

test_core: test_core.cpp $(CORE)/prisma_core.cpp $(CORE)/prisma_core.h
	$(CXX) -std=c++17 -Wall -Wextra -O1 -I$(CORE) test_core.cpp $(CORE)/prisma_core.cpp -o $@ -lm
	./$@

clean:
	rm -f test_core

.PHONY: clean
