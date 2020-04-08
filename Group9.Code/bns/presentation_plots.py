import sys
import matplotlib
import numpy as np
# matplotlib.use('Agg') <-- Uncomment if using windows or WSL
import matplotlib.pyplot as plt

# files = ["../logs/vanilla/ashetty71/vanilla_blocks_1_minutes_1000_miners_16_unsolicited_peers_500",
#          "../logs/vanilla/jzhu340/vanilla_blocks_1_minutes_1000_miners_16_solicited_peers_500",
#          "../logs/kadcast/ashetty71/kadcast_blocks_1_minutes_1000_miners_16_kadBeta_3_peers_500",
#          "../logs/mincast/ashetty71/mincast_noScore_blocks_1_minutes_1000_miners_16_kadBeta_3_peers_500",
#          "../logs/mincast/jzhu340/mincast_score_blocks_1_minutes_1000_miners_16_kadBeta_3_peers_500"]
# files = ["../logs/vanilla/ashetty71/vanilla_blocks_1_minutes_1000_miners_16_unsolicited_peers_500",
#          "../logs/vanilla/jzhu340/vanilla_blocks_1_minutes_1000_miners_16_solicited_peers_500",
#          "../logs/kadcast/ashetty71/kadcast_blocks_1_minutes_1000_miners_16_kadBeta_5_peers_500",
#          "../logs/mincast/ashetty71/mincast_noScore_blocks_1_minutes_1000_miners_16_kadBeta_5_peers_500",
#          "../logs/mincast/jzhu340/mincast_score_blocks_1_minutes_1000_miners_16_kadBeta_5_peers_500"]
# files = ["../logs/vanilla/ashetty71/vanilla_minutes_60_miners_16_solicitied_peers_500",
#          "../logs/kadcast/ashetty71/kadcast_minutes_60_miners_16_kadBeta_3_peers_500",
#          "../logs/mincast/ashetty71/mincast_noScore_minutes_60_miners_16_kadBeta_3_peers_500",
#          "../logs/mincast/jzhu340/mincast_score_minutes_60_miners_16_kadBeta_3_peers_500"]
# files = ["../logs/vanilla/ashetty71/vanilla_minutes_60_miners_16_solicitied_peers_500",
#          "../logs/kadcast/ashetty71/kadcast_minutes_60_miners_16_kadBeta_5_peers_500",
#          "../logs/mincast/ashetty71/mincast_noScore_minutes_60_miners_16_kadBeta_5_peers_500",
#          "../logs/mincast/jzhu340/mincast_score_minutes_60_miners_16_kadBeta_5_peers_500"]
# files = ["../logs/vanilla/adhrit/vanilla_minutes_180_miners_16_solicitied_peers_500",
#          "../logs/kadcast/adhrit/kadcast_minutes_180_miners_16_kadBeta_3_peers_500",
#          "../logs/mincast/adhrit/mincast_noScore_minutes_180_miners_16_kadBeta_3_peers_500",
#          "../logs/mincast/jzhu340/mincast_score_minutes_180_miners_16_kadBeta_3_peers_500"]
files = ["../logs/vanilla/adhrit/vanilla_minutes_180_miners_16_solicitied_peers_500",
         "../logs/kadcast/adhrit/kadcast_minutes_180_miners_16_kadBeta_5_peers_500",
         "../logs/mincast/ashetty71/mincast_noScore_minutes_180_miners_16_kadBeta_5_peers_500",
         "../logs/mincast/jzhu340/mincast_score_minutes_180_miners_16_kadBeta_5_peers_500"]

for fname in files:
    file = open(fname+".log", "r")
    mining_dict = {}
    firstBlockID = ""
    gotfirstBlockID = False
    propagation_dict = {}
    for x in file:
        split = x.strip().split(" ")
        if split[2] == "BNSBitcoinNode:NotifyNewBlock():":
            if split[5] == "Didn't":
                propagation_dict[str(split[1]+"_"+split[11])
                                 ] = float(split[0][1:-1])
            else:
                propagation_dict[str(split[1]+"_"+split[8])
                                 ] = float(split[0][1:-1])
        elif split[2] == "BNSMincastNode:SendBlock():" or split[2] == "BNSVanillaNode:SendBlockMessage():" or split[2] == "BNSKadcastNode:SendBlock():":
                # print(split[7])
            if split[7] not in mining_dict:
                mining_dict[split[7]] = str(split[1]+"_"+split[0][1:-1])
                if gotfirstBlockID == False:
                    firstBlockID = split[7]
                    gotfirstBlockID = True
    # print(propagation_dict)
    print(mining_dict)
    x = {}
    y = {}
    for k, v in sorted(propagation_dict.items(), key=lambda item: item[1]):
        blockID = k.split("_")[1]
        if blockID not in x:
            x[blockID] = [float(mining_dict[blockID].split("_")[1])]
            y[blockID] = [1]
        else:
            x[blockID].append(v)
            y[blockID].append(y[blockID][len(y[blockID])-1]+1)
    # print(x)
    # print(y)
    print(firstBlockID)
    item = x[firstBlockID]
    coverage = [float(number) / int(fname.split("_")[-1])
                for number in y[firstBlockID]]
    # coverage.append(coverage[-1])
    print(coverage)
    propagation_delay = [number - item[0] for number in item]
    # propagation_delay.append(2300)
    plt.plot(propagation_delay, coverage)

plt.yticks(np.arange(0, 1, step=0.05))
plt.xlabel("Propogation Delay")
plt.ylabel("Coverage")
# plt.legend(["Vanilla Unsolicited", "Vanilla Solicited", "KadCast kadBeta=3",
#             "MinCast noScore kadBeta=3", "MinCast Score kadBeta=3"])
# plt.legend(["Vanilla Unsolicited", "Vanilla Solicited", "KadCast kadBeta=5",
#             "MinCast noScore kadBeta=5", "MinCast Score kadBeta=5"])
# plt.legend(["Vanilla Solicited", "KadCast kadBeta=3",
#             "MinCast noScore kadBeta=3", "MinCast Score kadBeta=3"])
plt.legend(["Vanilla Solicited", "KadCast kadBeta=5",
            "MinCast noScore kadBeta=5", "MinCast Score kadBeta=5"])
plt.show()
