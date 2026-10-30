# Host test of the non-blocking serial output (ccs/serialRing.cpp). Runs on a PC (g++), not on the STM32:
#   make -C test/serialRing run

CXX ?= g++
CXXFLAGS = -std=c++11 -O1 -g -Wall -Wextra -Werror -DSERIALRING_HOST_TEST -I. -I../../ccs \
           -fsanitize=address,undefined -fno-sanitize-recover=all

ring_test: ring_test.cpp ../../ccs/serialRing.cpp ../../ccs/serialRing.h serialRing_hosttest.h terminal.h
	$(CXX) $(CXXFLAGS) -o $@ ring_test.cpp

run: ring_test
	./ring_test 1 3000000
	./ring_test 2 3000000
	./ring_test 3 2000000 long
	./ring_test 4 2000000 long

clean:
	rm -f ring_test

.PHONY: run clean
