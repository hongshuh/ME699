#!/bin/bash

g++ -O3 -o rdf.out rdf.cpp


for REDUCED_DENSITY in 1 2 3 4 5 6 7 8 9 
do 
    ./rdf.out ./MC_traj_redden_${REDUCED_DENSITY}.lammpstrj
    mv rdf.dat rdf_${REDUCED_DENSITY}.dat
done