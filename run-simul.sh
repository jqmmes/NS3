#!/bin/sh
#======================================================================
#
# INPUT ARGS
#
#======================================================================
# Usage
usage="Usage: run_simul.sh [options]
   --nodes:                 Number of Nodes (1 default) [1]
   --min-peers:             Mininum number of peers. [1]
   --discovery-timer:       Time between discovery lookups (in ms) [100]
   --min-discovery-timeout: Minimum discovery timeout (in s) [5]
   --max-discovery-timeout: Maximum discovery timeout (in s) [12]
   --min-idle-timeout       Minimum discovery idle timeout (in s) [20]
   --max-idle-timeout:      Maximum discovery idle timeout (in s) [20]
   --sim-duration:          Simulation time (in s) [3600]
   --first-join-time:       Last join time (in s) [0.1]
   --last-join-time:        Last join time (in s) [1]
   --gossip-b-initial:      Gossip Algorithm Binitial [1]
   --gossip-b:              Gossip Algorithm B [1]
   --gossip-f:              Gossip Algorithm F [1]
   --seed:                  Set Random Seed [1893]
   --out-file:		    Set output file [./out.txt]
   --path-to-waf:           Set path to waf executable (default: .)
   --help:		    Prompt usage"

# Default values
nodes="1"
min_peers="1"
discovery_timer="100"
min_discovery_timeout="5"
max_discovery_timeout="12"
min_idle_timeout="20"
max_idle_timeout="20"
sim_duration="3600"
first_join_time="0.1"
last_join_time="1"
gossip_b_initial="1"
gossip_b="1"
gossip_f="1"
seed="1893"
out_file="$(pwd -P)/out.txt"
path_to_waf="/home/hyrax/ns-allinone-3.24/ns-3.24/"

#
while [[ $# > 0 ]]
do
key="$1"

case $key in
   --nodes)
	nodes=$2
	shift   
   ;;
   --gossip-b)
	gossip_b=$2
	shift
   ;;
   --gossip-b-initial)
	gossip_b_initial=$2
	shift
   ;;
   --gossip-f)
	gossip_f=$2
	shift
   ;;
   --min-peers)
	min_peers="$2"
	shift
   ;;
   --sim-duration)
	sim_duration="$2"
	shift
   ;;
   --discovery-timer)
	discovery_timer="$2"
	shift
   ;;
   --min-idle-timeout)
	min_idle_timeout="$2"
	shift
   ;;
   --max-idle-timeout)
	max_idle_timeout="$2"
	shift
   ;;
   --min-discovery-timeout)
	min_discovery_timeout="$2"
	shift
   ;;
   --max-discovery-timeout)
	max_discovery_timeout="$2"
	shift
   ;;
   --first-join-time)
	first_join_time="$2"
	shift
   ;;
   --last-join-time)
	last_join_time="$2"
	shift
   ;;
   --seed)
	seed="$2"
	shift
   ;;
   --out-file)
	out_file="$2"
	shift
   ;;
   --path-to-waf)
	if [ -d "$2" ]; then
	  path_to_waf=$2
	else
	  echo "$2 is not a valid directory."
	  exit 1
	fi
	shift	
   ;;
   --help)
	echo -e "\n${usage}\n"
	exit 0
   ;;
   *)
	echo "Unknown option: $key"
	echo -e "\n${usage}\n"
	exit 1
   ;;

esac
shift # past argument or value
done

#======================================================================
#
# Run 
#
#======================================================================
if [ ! -f "${path_to_waf}/waf" ]; then
  echo "${path_to_waf}/waf does not exist. Are you sure this is the correct path to ns3/waf?"
  exit 1
fi


echo "Running simul with waf.. "

cp -r Piconet ${path_to_waf}

pushd ${path_to_waf}
./waf --run="scratch/Piconet/Piconet --Seed=$RANDOM --Nodes=$nodes --GossipBinitial=$gossip_b_initial --GossipB=$gossip_b --GossipF=$gossip_f" > $out_file; 
popd

echo "Finished running simul."
