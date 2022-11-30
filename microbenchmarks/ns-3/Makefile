EXEC=benchmarks
NS3_PATH=./ns-3/
NS3_VERSION="3.36.1"
NS3_ARGS= -D NS3_LOG_ENABLE -L ${NS3_PATH}/build/lib/ -I ${NS3_PATH}/build/include/
NS3_ARGS+=$(addprefix -l, $(subst lib,,$(subst .so,,$(notdir $(wildcard ${NS3_PATH}/build/lib/libns3*.so)))))
SRC1=src/main.cpp src/callbacks.cpp
SRC3=src/simplemix.cpp src/callbacks.cpp

all: benchmarks simplemix

benchmarks: $(SRC1)
	g++ -std=c++17 -fPIC -g  -D NS3_VERSION=\"${NS3_VERSION}\" $(NS3_ARGS) $(SRC1) -o $@  $(NS3_ARGS)
	@echo -e "\e[32mRun the following command before running benchmarks:\e[0m"
	@echo -e "\e[32mexport LD_LIBRARY_PATH=${NS3_PATH}/build/lib\e[0m"

simplemix: $(SRC3)
	g++ -std=c++17 -fPIC -g  -D NS3_VERSION=\"${NS3_VERSION}\" $(NS3_ARGS) $(SRC3) -o $@  $(NS3_ARGS)

run:  benchmarks
ifeq ($(ARGS),)
	$(error Missing ARGS parameter to run the simulator)
endif
	@export LD_LIBRARY_PATH=${NS3_PATH}/build/lib && ./benchmarks ${ARGS}

run_simplemix: simplemix
ifeq ($(ARGS),)
	$(error Missing ARGS parameter to run the simulator)
endif
	@export LD_LIBRARY_PATH=${NS3_PATH}/build/lib && ./simplemix ${ARGS}


clean:
	- rm benchmarks simplemix

.PHONY: clean
