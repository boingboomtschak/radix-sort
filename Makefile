CXXFLAGS=-std=c++17
RUN_EXT=run
ifeq ($(OS), Windows_NT)
	RUN_EXT = exe
endif

%.spv: %.comp
	glslc --target-env=vulkan-1.1 $< -o $@

all: easyvk/build/easyvk.o onesweep.o onesweep_driver.cpp
	$(CXX) $(CXXFLAGS) -L$(VULKAN_SDK)/Lib -lvulkan -Ieasyvk/src easyvk/build/easyvk.o onesweep.o onesweep_driver.cpp -o onesweep_driver.$(RUN_EXT)

onesweep.o: onesweep.h onesweep.cpp shaders/histogram.spv shaders/onesweep.spv
	$(CXX) $(CXXFLAGS) -Ieasyvk/src onesweep.cpp -c -o onesweep.o

easyvk/build/easyvk.o:
	make -C easyvk


