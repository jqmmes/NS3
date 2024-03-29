/**
 * Por implementar:
 * server_max_files
 * client_update_on_join
 **/


#ifndef RING_H
#define RING_H

using namespace ns3;
using namespace std;

enum update_type_t {server, client_on_request, client_timed};

/**
 * Parameters
 **/
bool use_churn = false; // Simulate Chrun effect
bool show_messages = false; // Show debug messages to screen
bool trace_to_file = false; // Trace results to files (each per node)
bool random_server_file_inc = false; // Increase number os files in server randomly
bool random_client_sleep = false; // Make client sleep duration random
bool random_server_sleep = false; // Make server sleep duration random
bool early_start = false; // Start all nodes in the first 1/10 of time.


uint32_t random_seed = 1893;

uint32_t node_number = 10; // Node number to use in the simulation
uint32_t simulation_time = 100; // Time to simulate (in seconds)
uint32_t server_fixed_file_increase = 5; // Fixed increase of the number of files in the server
uint32_t server_initial_file_count = 10; // Initial file count in the server
uint32_t server_file_increase_min = 1;  // If random_server_file_inc, minimum increase size
uint32_t server_file_increase_max = 10; // If random_server_file_inc, maximum increase size
uint32_t server_fixed_sleep_duration = 20; // Fixed sleep time.
uint32_t server_sleep_duration_min = 1;  // If random_server_sleep, minimum sleep size
uint32_t server_sleep_duration_max = 20; // If random_server_sleep, maximum sleep size
uint32_t server_max_files = 999999; // Max number of files generated by the server

uint32_t client_fixed_sleep_duration = 10; // Fixed client sleep duration
uint32_t client_sleep_duration_min = 1; // If random_client_sleep, minimum sleep time
uint32_t client_sleep_duration_max = 20; // If random_client_sleep, minimum sleep time
bool client_update_on_join = false; // When a client joins, it requests latest file list

string file_prefix("experiment"); // File name prefix, when trace_to_file is selected
string file_list_update_type = "server"; // server, client_on_request, client_timed
uint32_t file_list_update_time = 10; // if file_list_update_type == client_timed, 10 seconds.

string churn_intensity("low");

string application = "ring";
fstream trace_file;

map<string, vector<string>> avaliable_application_server_options {
  make_pair("ring", vector<string> {
    "server", 
    "client_on_request",
    "client_timed"
  }),
  make_pair("star", vector<string> {
    "server_bcast", 
    "server_master", 
    "client_master_on_request",
    "client_master_timed",
    "master_client_on_update",
    "master_client_timed"
  }),
  make_pair("purep2p", vector<string> {
    "server",
    "client_on_request",
    "client_timed"
  }),
  make_pair("wellformed", vector<string> {
    "server",
    "client_on_request",
    "client_timed"
  })
};

/**
 * Start the simulation for application T.
 **/
template <typename T>
void StartSimulation(NodeContainer nodes, Ptr<UniformRandomVariable> randomGen, VirtualDiscovery *discovery){
  NS_ASSERT(simulation_time > 10);
  simulation_time = (uint32_t)max((uint32_t)round(simulation_time/node_number), simulation_time);

  // Init server configuration
  Ptr<MyServer> server = CreateObject<MyServer> ();
  server->Setup(show_messages, random_server_file_inc, random_server_sleep, server_fixed_file_increase, server_initial_file_count, server_file_increase_min, server_file_increase_max, server_sleep_duration_min, server_sleep_duration_max, server_fixed_sleep_duration, file_list_update_type, application, trace_to_file, &trace_file, randomGen);
  NS_ASSERT(node_number > 0);
  nodes.Get (0)->AddApplication (server);
  server->SetStartTime (Seconds (0.));
  server->SetStopTime (MilliSeconds (simulation_time*1000+(1000))); //Make server stop 1 second after all nodes to Call Simulator::Stop()
  uint32_t start_time;
  // Init nodes configuration
  Ptr<T> node;
  for (int x = 1; x < node_number; x++){
    NS_ASSERT(x <= node_number);
    node = CreateObject<T> ();
    node->Setup(discovery, show_messages, use_churn, random_client_sleep, client_sleep_duration_min, client_sleep_duration_max, client_fixed_sleep_duration, file_list_update_type, file_list_update_time, simulation_time, randomGen, trace_to_file, &trace_file, churn_intensity);
    nodes.Get (x)->AddApplication (node);
    //uint32_t start_time = rand() % (simulation_time*1000-9);
    if (early_start){
      start_time = randomGen->GetInteger(1,((simulation_time-5)*1000)/10);
    }else{
      start_time = randomGen->GetInteger(1,(simulation_time-5)*1000);
      //uint32_t start_time = randomGen->GetInteger(1,INT_MAX) % ((simulation_time-5)*1000);
    }
    //Time st_time = MilliSeconds (start_time);
    node->SetStartTime (MilliSeconds (start_time));
    //if (!use_churn) 
    node->SetStopTime (MilliSeconds (simulation_time*1000));
    // else{
    //   uint32_t end_time = (rand() % ((simulation_time*1000-9)-start_time)) + start_time;
    //   node->SetStopTime (MilliSeconds (end_time));
    // }
  }

  GlobalValue::Bind ("SimulatorImplementationType",  StringValue ("ns3::RealtimeSimulatorImpl"));
  Simulator::Stop(Seconds(simulation_time));
  Simulator::Run();
}

/**
 * Parse Configuration file.
 **/
bool ConfigurationParser(string filename){
  fstream fs;
  fs.open(filename.c_str(), ios::in);
  if(fs.is_open()){
    regex argParam("[A-Za-z0-9_]+:[ ]+[A-Za-z0-9_-]+(.*#.*)?");
    regex onlyParam("[A-Za-z0-9_]+(.*#.*)?");
    regex isComment("#.*|$");
    uint16_t l = 1;
    for(string line; getline(fs, line); ){
      boost::trim(line);
      if (regex_match(line, argParam)){
        uint32_t end_param = line.find(':');
        string param = line.substr(0, end_param);
        boost::trim_right(param);
        end_param++;
        string value = line.substr(end_param, (min(line.find('#'), line.length())-end_param));
        boost::trim(value);
        if(param == "node_number") node_number = stoi(value);
        else if(param == "simulation_time") simulation_time = stoi(value);
        else if(param == "server_fixed_file_increase") server_fixed_file_increase = stoi(value);
        else if(param == "server_initial_file_count") server_initial_file_count = stoi(value);
        else if(param == "server_sleep_duration_min") server_sleep_duration_min = stoi(value);
        else if(param == "server_sleep_duration_max") server_sleep_duration_max = stoi(value);
        else if(param == "server_file_increase_min") server_file_increase_min = stoi(value);
        else if(param == "server_file_increase_max") server_file_increase_max = stoi(value);
        else if(param == "client_fixed_sleep_duration") client_fixed_sleep_duration = stoi(value);
        else if(param == "client_sleep_duration_min") client_sleep_duration_min = stoi(value);
        else if(param == "client_sleep_duration_max") client_sleep_duration_max = stoi(value);
        else if(param == "server_fixed_sleep_duration") server_fixed_sleep_duration = stoi(value);
        else if(param == "file_list_update_type") file_list_update_type = value;
        else if(param == "file_list_update_time") file_list_update_time = stoi(value);
        else if(param == "file_prefix") file_prefix = value;
        else if(param == "application") application = value;
        else if(param == "set_seed") random_seed = stoi(value);
        else if(param == "churn_intensity"){
          if (value == "low" || value == "medium" || value == "high"){
            churn_intensity = value;
          }else{
            cerr << "churn_intensity: " << value << " is invalid (low, medium, high).\nUsing low by default.\n";
          }
        }
        else cerr << "Warning: " << param << " is not a valid parameter (" << filename << ": " << l << ")" << endl;
      }else if(regex_match(line, onlyParam)){
        string param = line.substr(0, min(line.find('#'),line.length()));
        boost::trim_right(param);
        if(param == "use_churn") use_churn = true;
        else if(param == "early_start") early_start = true;
        else if(param == "show_messages") show_messages = true;
        else if(param == "trace_to_file") trace_to_file = true;
        else if(param == "random_server_file_inc") random_server_file_inc = true;
        else if(param == "random_client_sleep") random_client_sleep = true;
        else if(param == "random_server_sleep") random_server_sleep = true;
        else cerr << "Warning: " << param << " is not a valid parameter (" << filename << ": " << l << ")" << endl;
      }else if(regex_match(line, isComment)){
        continue;
      }
      else{
        cerr << "Warning: \"" << line <<"\"" << " is not a valid expression (" << filename << ": " << l << ")" << endl;
      }
      l++;
    }
  }
  fs.close();
  auto it = avaliable_application_server_options.find(application);
  if (it != avaliable_application_server_options.end()){
    vector<string> server_options = avaliable_application_server_options[application];
    auto fp = find(begin(server_options), end(server_options), file_list_update_type);
    if (fp == end(server_options)){
      cerr << "Error: Invalid file_list_update_type. Avaliable options for \"" << application << "\" are:" << endl;
      for(string fn : server_options){
        cerr << fn << endl;
      }
      return false;
    }
  }else{
    cerr << "Error: Invalid application. Avaliable options are:" << endl;
    for(it = avaliable_application_server_options.begin(); it != avaliable_application_server_options.end(); it++){
      cerr << it->first << endl;
    }
    return false;
  }
  if (trace_to_file){
    stringstream trace_file_name;
    trace_file_name << file_prefix << "_" << application << ".out";
    trace_file.open(trace_file_name.str(), ios::out);
    trace_file.write("from, to, file, hops, time(s)\n", 31);
  }

  cout << 
  "####################################################\n" <<
  "Application: " << application << endl <<
  "Duration: " << simulation_time << "s\n" <<
  "Node Number: " << node_number << endl <<
  "Initial File Count: " << server_initial_file_count << endl <<
  "####################################################\n\n";

  return true;
}

#endif