# Host tests: code without hardware access (or with it simulated), built and run on the PC.
#   make -C test/host

CXX      ?= g++
CXXFLAGS  = -std=c++11 -Wall -Wextra -Werror -O1 -g -I../../ccs -fsanitize=address,undefined -fno-sanitize-recover=all

all: run

v2gtp_test: v2gtp_test.cpp ../../ccs/v2gtp.cpp ../../ccs/v2gtp.h
	$(CXX) $(CXXFLAGS) -o $@ v2gtp_test.cpp ../../ccs/v2gtp.cpp

qcaframe_test: qcaframe_test.cpp ../../ccs/qcaframe.cpp ../../ccs/qcaframe.h
	$(CXX) $(CXXFLAGS) -o $@ qcaframe_test.cpp ../../ccs/qcaframe.cpp

# ccs/qca7000.cpp with the SPI, its DMA and the QCA7000 simulated (qca7000_hosttest.h)
qca7000_test: qca7000_test.cpp qca7000_hosttest.h ../../ccs/qca7000.cpp ../../ccs/qca7000.h ../../ccs/qcaframe.cpp
	$(CXX) $(CXXFLAGS) -DQCA7000_HOST_TEST -I. -o $@ qca7000_test.cpp ../../ccs/qcaframe.cpp

run: v2gtp_test qcaframe_test qca7000_test
	./v2gtp_test
	./qcaframe_test
	./qca7000_test

clean:
	rm -f v2gtp_test qcaframe_test qca7000_test

.PHONY: all run clean
