import matplotlib.pyplot as plt
import numpy as np
import datetime

def time_str_to_timestamp(time_str):
    """Converts a time string in the format 'minute:second' to a timestamp."""

    minutes, seconds = map(int, time_str.split(':'))
    return datetime.timedelta(minutes=minutes, seconds=seconds).total_seconds()

## Read the time data and plot vs density
total_density = []
for density in [0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9]:
    time_list = []
    for seed in [0,42,123,666]:
        with open(f'./time_dat/mc_time_{density}_{seed}.dat','r') as f:
            lines = f.readlines()
            ## Split first line
            data = lines[0].split()
            time = data[2].split('.')[0]
            time = time_str_to_timestamp(time)
            
            time_list.append(time)
    total_density.append(time_list)

time_array = np.array(total_density)
print(time_array.shape)

## Plot mean and std versus density
plt.figure()
plt.errorbar([0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9], time_array.mean(axis=1), yerr=time_array.std(axis=1), fmt='o')
plt.xlabel('Density')
plt.ylabel('Time (s)')

plt.savefig('./figure/time.pdf')
        