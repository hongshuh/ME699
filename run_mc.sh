#!/bin/bash
 
for REDUCED_DENSITY in 0.2
do
    for SEED in 42 666 123 0
    do
        g++ -O3 -o mc_${REDUCED_DENSITY}.out mc.cpp



        # Output the values being used
        echo "Running with seed: $SEED and reduced density: $REDUCED_DENSITY"

        # Run the Monte Carlo application and save timing and output log
        /usr/bin/time -o mc_time_${REDUCED_DENSITY}_${SEED}.dat ./mc_${REDUCED_DENSITY}.out "$SEED" "$REDUCED_DENSITY" | tee mc_${REDUCED_DENSITY}_${SEED}.log
    done
done
