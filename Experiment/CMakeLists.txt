cmake_minimum_required(VERSION 3.2)
project(Experiment)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    Experiment.cc
    Experiment.h
    ExperimentApp.cc
    ExperimentApp.h
    ExperimentAppWifiDirect.cc
    ExperimentAppWifiDirect.h)

add_executable(Experiment ${SOURCE_FILES})