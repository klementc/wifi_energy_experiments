SG_ARGS= -L ./simgrid/build/lib -lsimgrid -I ./simgrid/build/include -I ./simgrid/include -I ./simgrid/ -I ./simgrid/build/


all: wifisim

wifisim: wifisim.cpp
	g++ --std=c++17 -fPIC $(SG_ARGS) $^ -o $@ $(SG_ARGS)

run: wifisim
ifeq ($(ARGS),)
	$(error Missing ARGS parameter to run the simulator)
endif
	export LD_LIBRARY_PATH=./simgrid/build/lib:./ns-3/ns-3/build/lib && ./wifisim --cfg=network/model:CM02 --cfg=network/crosstraffic:0 --log="link_energy_wifi.thres:debug" ${ARGS}

clean:
	- rm wifisim

