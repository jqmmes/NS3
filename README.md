# NS3
NS3 simulations

Clone to 'ns3/scratch/'


## Requirements:

* ns-3.24+
* gcc-4.9+
* boost-1.57.0+

## Configure:
	CXXFLAGS="-std=c++0x" ./waf --build-profile=optimized configure

## Run:
	./waf --run <ns3-program> --command-template="%s <args>"

## E.g.:
	Joaquim :: ~/Work/ns3 % ./waf --run="scratch/Experiment/Experiment --Nodes=2 --Servers=1 --Scenario=1 --Seed=$RANDOM"