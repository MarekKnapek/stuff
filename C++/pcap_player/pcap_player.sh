#!/bin/bash

g++ pcap_player.cpp -O2 -s -DNDEBUG -std=c++17 -o pcap_player.elf
#g++ pcap_player.cpp -O0 -g -DDEBUG -std=c++17 -o pcap_player.elf
