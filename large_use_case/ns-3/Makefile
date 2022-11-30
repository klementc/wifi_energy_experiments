NS3_PATH=./ns-3/
NS3_VERSION="3.36.1"
NS3_ARGS= -D NS3_LOG_ENABLE -L ${NS3_PATH}/build/lib/ -I ${NS3_PATH}/build/include/
NS3_ARGS+=$(addprefix -l, $(subst lib,,$(subst .so,,$(notdir $(wildcard ${NS3_PATH}/build/lib/libns3*.so)))))


all: wifiSimNS

wifiSimNS: wifisimNS.cpp
	g++ -std=c++17 -fPIC -g -D NS3_VERSION=\"${NS3_VERSION}\" $(NS3_ARGS) $^ -o $@ $(NS3_ARGS)

run:  wifiSimNS
ifeq ($(ARGS),)
	$(error Missing ARGS parameter to run the simulator)
endif
	@export LD_LIBRARY_PATH=${NS3_PATH}/build/lib && ./wifiSimNS ${ARGS}


clean:
	- rm wifiSimNS

.PHONY: clean
