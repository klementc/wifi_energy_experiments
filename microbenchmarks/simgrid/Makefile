EXEC=benchmarks
BUILD_DIR=build
SG_ARGS= -L ./simgrid/${BUILD_DIR}/lib -lsimgrid -I ./simgrid/${BUILD_DIR}/include -I ./simgrid/include -I ./simgrid/ -I ./simgrid/${BUILD_DIR}/
SRC3=src/simplemix.cpp

all: benchmarks simplemix

benchmarks: src/benchmarks.cc
	g++ --std=c++17 -fPIC $(SG_ARGS) $^ -o $@ $(SG_ARGS)

simplemix: $(SRC3)
	g++ --std=c++17 -fPIC $(SG_ARGS) $^ -o $@ $(SG_ARGS)

run: benchmarks
ifeq ($(ARGS),)
	$(error Missing ARGS parameter to run the simulator)
endif
	export LD_LIBRARY_PATH=./simgrid/${BUILD_DIR}/lib:./ns-3/ns-3/build/lib && ./benchmarks --cfg=network/model:CM02 --cfg=network/crosstraffic:0 ${ARGS}

run_simplemix: simplemix
ifeq ($(ARGS),)
	$(error Missing ARGS parameter to run the simulator)
endif
	export LD_LIBRARY_PATH=./simgrid/${BUILD_DIR}/lib && ./simplemix --cfg=network/model:CM02 --cfg=network/crosstraffic:0 ${ARGS}


clean:
	- rm benchmarks simplemix

