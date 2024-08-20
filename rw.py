import os
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
np.set_printoptions(suppress=True)
from sklearn.linear_model import LinearRegression
from sklearn.metrics import r2_score
from sklearn import tree
from sklearn.tree import DecisionTreeClassifier
from sklearn.ensemble import RandomForestClassifier
from sklearn.decomposition import PCA
from sklearn import linear_model
from scipy.optimize import curve_fit

import multiprocessing as mp
import operator


file_list = []
length_list = {}
unique_num_list = {}
windows_list = {}
cache_size_list = {}
chopt_cost_list = {}
belady_cost_list = {}
static_cost_list = {}
lru_cost_list = {}
lfu_cost_list = {}
tinylfu_cost_list = {}
wtinylfu_cost_list = {}
tinylfu_only_cost_list = {}
slru_cost_list = {}

matrix = {}

columns_list = {}

legend_list=["r/w", "isScan", "rd", "age", "max size/64", "max size/32", "max size/16", "max size/8","max size/4", "max size/2", "max size", "2*max size", "4*max size", "8*max size"]

original_perf = {}
simplified_perf = {}

def fit(self, X, a, b, c, d):
    x,y,z=X
    return a*x+ b*np.log(y) + c*np.log(z) + d

def r2(y1, y2):
	return round(1-np.var(y1-y2)/np.var(y1), 3)

def print_opt_lru_compare(catalog,granularity):
	#plt.figure(figsize=(20,8))
	for i in range(len(file_list)):
		fname = file_list[i]
		print(fname)
		length = length_list[fname]
		unique_num = unique_num_list[fname]

		c=cache_size_list[fname]
		chopt=chopt_cost_list[fname]
		belady=belady_cost_list[fname]
		static=static_cost_list[fname]
		lru=lru_cost_list[fname]
		lfu=lfu_cost_list[fname]
		tinylfu=tinylfu_cost_list[fname]
		wtinylfu=wtinylfu_cost_list[fname]
		tinylfu_only=tinylfu_only_cost_list[fname]
		slru=slru_cost_list[fname]
		
		
		#p=np.asarray(original_perf[fname])
		#s=np.asarray(simplified_perf[fname])

		#plt.subplot(2, len(file_list), i+1)
		plt.subplot(2,1,1)
		plt.ylim(0, 11)
		#plt.xlabel("cache size")
		plt.plot(c, chopt/length, label="CHOPT", marker="o")
		plt.plot(c, belady/length, label="Belady", marker='x')
		plt.plot(c, static/length, label="Static", marker='v')
		plt.plot(c, lru/length, label="LRU", marker='s')
		plt.plot(c, lfu/length, label="LFU", marker='*')
		plt.plot(c, tinylfu/length, label="TinyLFU", marker='^')
		
		#plt.hlines(5, c[0], c[-1], colors='grey', alpha=0.5, linestyles='dashed')
		#plt.hlines(10, c[0], c[-1], colors='grey', alpha=0.5, linestyles='dashed')
		plt.legend()
		#if i == 0:
		plt.ylabel("avg cost per access")
		plt.title(fname  + " " + str(granularity))

		plt.subplot(2, 1, 2)
		#plt.ylim(0, 11)
		plt.plot(c, chopt/length, label="CHOPT", marker="o")
		plt.plot(c, tinylfu/length, label="TinyLFU", marker='^')
		#plt.plot(c, wtinylfu/length, label="W-TinyLFU", marker='x')
		#plt.plot(c, tinylfu_only/length, label="TinyLFU Only", marker='s')
		#plt.plot(c, slru/length, label="SLRU", marker='v')


		plt.xlabel("cache size")
		#plt.hlines(5, c[0], c[-1], colors='grey', alpha=0.5, linestyles='dashed')
		#plt.hlines(10, c[0], c[-1], colors='grey', alpha=0.5, linestyles='dashed')
		plt.legend()
		plt.ylabel("avg cost per access")

		if not os.path.exists("simulation/result/"+catalog+granularity):
			os.makedirs("simulation/result/"+catalog+granularity)
		plt.savefig("simulation/result/"+catalog+granularity+fname+".png")
		plt.clf()



def load_simulation_file(root_dir, catalog, granularity, option):
	_dir = root_dir + catalog + granularity + option
	temp = os.listdir(_dir)
	#temp = ["trace_fft"]
	_list = []
	for item in temp:
		if option == "online/":
			file_list.append(item)
		_list.append(item)

	for fname in _list:
		# Output matrix file: for each source file
		# filename, length, unique num, 
		# "windows", windows, 
		# "cache size", cache size, 
		# for online: 
		# 	"lru cost", lru_cost[cache_size],
		# 	"lfu cost", lfu_cost[cache_size],
		# 	"tinylfu cost", tinylfu_cost[cache_size],
		# for offline:
		# 	"chopt cost", chopt_cost[cache_size],
		# 	"belady cost", belady_cost[cache_size],
		# 	"static cost", static_cost[cache_size],
		
		file = open(_dir+fname)
		line = file.readline().split("\n")[0].split(" ")
		print(fname, granularity, line)
		length = int(line[3])
		length_list[fname] = length
		unique_num = int(line[6])
		unique_num_list[fname] = unique_num

		line = file.readline().split("\n")[0].split(" ")
		windows = np.array(line[1:-1]).astype(int)
		windows_list[fname] = windows

		line = file.readline().split("\n")[0].split(" ")
		cache_sizes = np.array(line[2:-1]).astype(int)
		cache_size_list[fname] = cache_sizes

		if option == "online/":
			line = file.readline().split("\n")[0].split(" ")
			lru_cost = np.array(line[2:-1]).astype(int)
			lru_cost_list[fname] = lru_cost

			line = file.readline().split("\n")[0].split(" ")
			lfu_cost = np.array(line[2:-1]).astype(int)
			lfu_cost_list[fname] = lfu_cost

			line = file.readline().split("\n")[0].split(" ")
			tinylfu_cost = np.array(line[2:-1]).astype(int)
			tinylfu_cost_list[fname] = tinylfu_cost

			line = file.readline().split("\n")[0].split(" ")
			wtinylfu_cost = np.array(line[2:-1]).astype(int)
			wtinylfu_cost_list[fname] = wtinylfu_cost

			line = file.readline().split("\n")[0].split(" ")
			tinylfu_only_cost = np.array(line[3:-1]).astype(int)
			tinylfu_only_cost_list[fname] = tinylfu_only_cost

			line = file.readline().split("\n")[0].split(" ")
			slru_cost = np.array(line[2:-1]).astype(int)
			slru_cost_list[fname] = slru_cost

			print(fname, "online loaded")

		elif option == "offline/":
			line = file.readline().split("\n")[0].split(" ")
			chopt_cost = np.array(line[2:-1]).astype(int)
			chopt_cost_list[fname] = chopt_cost

			line = file.readline().split("\n")[0].split(" ")
			belady_cost = np.array(line[2:-1]).astype(int)
			belady_cost_list[fname] = belady_cost

			line = file.readline().split("\n")[0].split(" ")
			static_cost = np.array(line[2:-1]).astype(int)
			static_cost_list[fname] = static_cost

			print(fname, "offline loaded")

if __name__ == '__main__':
	root_dir = "simulation/"
	catalog = "short/"
	granularity = "max/"
	load_simulation_file(root_dir, catalog, granularity, "online/")
	load_simulation_file(root_dir, catalog, granularity, "offline/")
	print_opt_lru_compare(catalog, granularity)

	
	