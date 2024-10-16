import numpy as np
import matplotlib.pyplot as plt



# Read the data from the log file
for redden in [1,2,3,4,5,6,7,8,9]:
    with open(f'./rdf_file/rdf_{redden}.dat','r') as f:
        lines = f.readlines()
        dist = []
        gr = []
        num_int = []
        for line in lines[1:]:
            data = line.split()
            dist.append(float(data[0]))
            gr.append(float(data[-1]))
            num_int.append(float(data[1]))
        plt.figure()
        plt.plot(dist,gr)
        plt.xlabel('Distance')
        plt.ylabel('g(r)')
        plt.savefig(f'./figure/rdf_{redden}.png')

        plt.figure()
        plt.plot(dist,num_int)
        plt.xlabel('Distance')
        plt.ylabel('Number of integral')
        plt.savefig(f'./figure/num_int_{redden}.png')
