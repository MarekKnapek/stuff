#!/bin/bash

g++ pcap_capture.cpp -O2 -s -DNDEBUG -std=c++17 -o pcap_capture.elf
#g++ pcap_capture.cpp -O0 -g -DDEBUG -std=c++17 -o pcap_capture.elf
