import os
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
np.set_printoptions(suppress=True)
import multiprocessing as mp


def plot(catalog, granularity, item, cache_size_list, window_list, window_result_list, counter_list, bf_list, cbf_list, w_list, prob_list, protect_list, slru_list):
	print(item)
	if not os.path.exists("tinylfu/result/"+catalog+granularity):
		os.makedirs("tinylfu/result/"+catalog+granularity)
	for cache_size in cache_size_list:
		plt.plot((np.arange(10)+1)/100, window_result_list[cache_size], label=str(cache_size))
	plt.xlabel("window size/cache size")
	plt.title(item  + " " + str(granularity))
	plt.legend()
	plt.savefig("tinylfu/result/" + catalog + granularity + item + "_window.png")
	plt.clf()

	for cache_size in cache_size_list:
		plt.plot(np.array(counter_list[cache_size]), bf_list[cache_size], label=str(cache_size))
	plt.xlabel("BF size")
	plt.title(item  + " " + str(granularity))
	plt.legend()
	plt.savefig("tinylfu/result/" + catalog + granularity + item + "_bf.png")
	plt.clf()

	for cache_size in cache_size_list:
		plt.plot(np.array(counter_list[cache_size]), cbf_list[cache_size], label=str(cache_size))
	plt.xlabel("CBF size")
	plt.title(item  + " " + str(granularity))
	plt.legend()
	plt.savefig("tinylfu/result/" + catalog + granularity + item + "_cbf.png")
	plt.clf()

	for cache_size in cache_size_list:
		plt.plot(np.array(counter_list[cache_size]), w_list[cache_size], label=str(cache_size))
	plt.xlabel("W")
	plt.title(item  + " " + str(granularity))
	plt.legend()
	plt.savefig("tinylfu/result/" + catalog + granularity + item + "_w.png")
	plt.clf()

	for cache_size in cache_size_list:
		plt.plot((np.arange(len(slru_list[cache_size]))+1)/10, slru_list[cache_size], label=str(cache_size))
	plt.xlabel("Prob/cache size")
	plt.title(item  + " " + str(granularity))
	plt.legend()
	plt.savefig("tinylfu/result/" + catalog + granularity + item + "_slru.png")
	plt.clf()


def load():
	catalog = "short/"
	granularity = "max/"
	_dir = "tinylfu/" + catalog + granularity

	dirs = os.listdir(_dir)

	for item in dirs:
		cache_size_list = []
		window_list = {}
		window_result_list = {}
		counter_list = {}
		bf_list = {}
		cbf_list = {}
		w_list = {}
		prob_list = {}
		protect_list = {}
		slru_list = {}

		files = os.listdir(_dir+item)
		for file in files:
			f=open(_dir+item+"/"+file)
			line = f.readline()
			if line == "":
				continue
			cache_size = int(line.split("\n")[0])
			cache_size_list.append(cache_size)

			line = f.readline()
			window = np.array(line.split("\n")[0].split(" ")[0:-1]).astype(int)
			window_list[cache_size] = window

			line = f.readline()
			window_result = np.array(line.split("\n")[0].split(" ")[0:-1]).astype(int)
			window_result_list[cache_size] = window_result

			line = f.readline()
			counter = np.array(line.split("\n")[0].split(" ")[0:-1]).astype(int)
			counter_list[cache_size] = counter

			line = f.readline()
			bf = np.array(line.split("\n")[0].split(" ")[0:-1]).astype(int)
			bf_list[cache_size] = bf

			line = f.readline()
			cbf = np.array(line.split("\n")[0].split(" ")[0:-1]).astype(int)
			cbf_list[cache_size] = cbf

			line = f.readline()
			w = np.array(line.split("\n")[0].split(" ")[0:-1]).astype(int)
			w_list[cache_size] = w

			line = f.readline()
			prob = np.array(line.split("\n")[0].split(" ")[0:-1]).astype(int)
			prob_list[cache_size] = prob

			line = f.readline()
			protect = np.array(line.split("\n")[0].split(" ")[0:-1]).astype(int)
			protect_list[cache_size] = protect

			line = f.readline()
			slru = np.array(line.split("\n")[0].split(" ")[0:-1]).astype(int)
			slru_list[cache_size] = slru

		plot(catalog, granularity, item, cache_size_list, window_list, window_result_list, counter_list, bf_list, cbf_list, w_list, prob_list, protect_list, slru_list)
			

if __name__ == '__main__':
	load()