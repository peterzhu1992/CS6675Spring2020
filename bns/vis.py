import matplotlib.pyplot as plt
fname = "./logs/mincast_30_20"
file1 = open(fname+".log", "r")
mining_dict = {}
propagation_dict = {}
for x in file1:
    split = x.strip().split(" ")
    if split[2] == "BNSBitcoinNode:NotifyNewBlock():":
        if split[5] == "Didn't":
            propagation_dict[str(split[1]+"_"+split[11])
                             ] = float(split[0][1:-1])
        else:
            propagation_dict[str(split[1]+"_"+split[8])
                             ] = float(split[0][1:-1])
    elif split[2] == "BNSMincastNode:SendBlock():":
            # print(split[7])
        if split[7] not in mining_dict:
            mining_dict[split[7]] = str(split[1]+"_"+split[0][1:-1])
print(propagation_dict)
print(mining_dict)
x = {}
y = {}
for item in propagation_dict.items():
    print(item)
    blockID = item[0].split("_")[1]
    if blockID not in x:
        x[blockID] = [float(mining_dict[blockID].split("_")[1])]
        y[blockID] = [1]
    else:
        x[blockID].append(item[1])
        y[blockID].append(y[blockID][len(y[blockID])-1]+1)
print(x)
print(y)
fig, ax = plt.subplots(2)
fig.suptitle('Vertically stacked subplots')

for item in x.items():
    coverage = [float(number / int(fname.split("_")[-1]))
                for number in y[item[0]]]
    propagation_delay = [number - item[1][0] for number in item[1]]
    ax[0].plot(propagation_delay, coverage)
    ax[1].plot(item[1], coverage)

ax.flat[0].set(xlabel="Propogation Delay", ylabel="Coverage")
ax.flat[1].set(xlabel="Simulation Time", ylabel="Coverage")
plt.legend(x.keys())
plt.show()