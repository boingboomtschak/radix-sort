import argparse, os, logging, sys, csv
import scienceplots
import numpy as np
import matplotlib.pyplot as plt

parser = argparse.ArgumentParser (description='Plotting results of Onesweep radix sort')
parser.add_argument("path", help="Path to .sortperf file")
args = vars(parser.parse_args())

format = "%(levelname)s : %(message)s"
logging.basicConfig(format=format, level=logging.INFO, datefmt="%H:%M:%S") 


path = args['path']
if not os.path.exists(path):
    logging.warning(f"File '{path}' does not exist! Exiting...")
    sys.exit(1)
if not os.path.split(path)[-1].endswith('.sortperf'):
    logging.warning(f"File '{path}' is not a .sortperf file! Exiting...")
    sys.exit(1)

data_sizes = []
hist = []
bin1 = []
bin2 = []
bin3 = []
bin4 = []
with open(path, 'r') as fp:
    reader = csv.reader(fp)
    for row in reader:
        data_sizes.append(int(row[0]))
        hist.append(float(row[2]))
        bin1.append(float(row[4]))
        bin2.append(float(row[6]))
        bin3.append(float(row[8]))
        bin4.append(float(row[10]))

plt.style.use(['science'])
plt.rcParams['svg.fonttype'] = 'none'
plt.rcParams['font.family'] = 'Linux Libertine'
plt.rcParams['text.usetex'] = False
plt.rcParams['legend.frameon'] = True
fig, ax = plt.subplots(figsize=(6, 5))
plt.stackplot(range(len(data_sizes)), hist, bin1, bin2, bin3, bin4, labels=['Histogram', 'Binning 1', 'Binning 2', 'Binning 3', 'Binning 4'])
plt.xticks(range(len(data_sizes)), labels=data_sizes, rotation=90)
plt.xlim(0, len(data_sizes) - 1)
plt.ylim(0.0, 100.0)
plt.title(f"Time taken for each Onesweep kernel\n{os.path.basename(path)}")
plt.xlabel("Data size (bytes)")
plt.ylabel("Time taken (percentage)")
plt.legend(loc='upper left', framealpha=0.7)
plt.tight_layout()
plt.grid()
graph_path = os.path.splitext(path)[0]+'.png'
logging.info(f"Saving figure to '{graph_path}'")
plt.savefig(graph_path, bbox_inches='tight', dpi=300)
plt.show()