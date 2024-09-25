CXXFLAGS=-std=c++17
RUN_EXT=run
ifeq ($(OS), Windows_NT)
	RUN_EXT = exe
endif

all: easyvk radix-sort.o radix-sort-driver.cpp
	$(CXX) $(CXXFLAGS) -L$(VULKAN_SDK)/Lib -lvulkan -Ieasyvk/src easyvk/build/easyvk.o radix-sort.o radix-sort-driver.cpp -o radix-sort-driver.$(RUN_EXT)

radix-sort.o: radix-sort.h radix-sort.cpp
	$(CXX) $(CXXFLAGS) -Ieasyvk/src radix-sort.cpp -c -o radix-sort.o

easyvk:
	make -C easyvk

%.spv: %.comp
	glslc $< -o $@