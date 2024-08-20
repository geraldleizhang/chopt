import os
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
np.set_printoptions(suppress=True)
import multiprocessing as mp

def plot(catalog, granularity, item, cache_size_list, window_list, custom_lru, custom_slru, custom_opt, wtinylfu_list):
	print(item)
	if not os.path.exists("custom/result/"+catalog+granularity):
		os.makedirs("custom/result/"+catalog+granularity)
	if not os.path.exists("custom/result/"+catalog+granularity+item):
		os.makedirs("custom/result/"+catalog+granularity+item)

	labels = ["no decay, count before", "no decay, count after", "decay, count before", "decay, count after"]
	markers = ["x", "^", "1", "2"]

	for cache_size in cache_size_list:
		window_size = window_list[cache_size]

		for i in range(4):
			plt.plot(window_size, custom_lru[i][cache_size], label=labels[i], marker = markers[i])
			# plt.plot(window_size, custom_slru[i][cache_size], label="slru " + labels[i])
			# plt.plot(window_size, custom_opt[i][cache_size], label="opt " + labels[i])			
		plt.plot(window_size, wtinylfu_list[cache_size], label="wtinylfu", marker='v')
		plt.xlabel("window lru size")
		plt.ylabel("latency")
		plt.yscale("log")
		plt.title(item + " " + str(cache_size) + " lru")
		plt.legend()
		plt.savefig("custom/result/"+catalog+granularity+item+"/"+str(cache_size)+"_lru.png")
		plt.close()

		for i in range(4):
			plt.plot(window_size, custom_slru[i][cache_size], label=labels[i], marker = markers[i])
			# plt.plot(window_size, custom_slru[i][cache_size], label="slru " + labels[i])
			# plt.plot(window_size, custom_opt[i][cache_size], label="opt " + labels[i])			
		plt.plot(window_size, wtinylfu_list[cache_size], label="wtinylfu", marker='v')
		plt.xlabel("window lru size")
		plt.ylabel("latency")
		plt.yscale("log")
		plt.title(item + " " + str(cache_size) + " slru")
		plt.legend()
		plt.savefig("custom/result/"+catalog+granularity+item+"/"+str(cache_size)+"_slru.png")
		plt.close()

		for i in range(4):
			plt.plot(window_size, custom_opt[i][cache_size], label=labels[i], marker = markers[i])
			# plt.plot(window_size, custom_slru[i][cache_size], label="slru " + labels[i])
			# plt.plot(window_size, custom_opt[i][cache_size], label="opt " + labels[i])			
		plt.plot(window_size, wtinylfu_list[cache_size], label="wtinylfu", marker='v')
		plt.xlabel("window lru size")
		plt.ylabel("latency")
		plt.yscale("log")
		plt.title(item + " " + str(cache_size) + " opt")
		plt.legend()
		plt.savefig("custom/result/"+catalog+granularity+item+"/"+str(cache_size)+"_opt.png")
		plt.close()


		# plt.plot(window_size, custom_lru[cache_size], label="custom lru", marker='o')
		# plt.plot(window_size, custom_slru[cache_size], label="custom slru", marker='x')
		# plt.plot(window_size, custom_opt[cache_size], label="custom opt", marker='^')
		# plt.plot(window_size, custom_decay_lru[cache_size], label="decay custom lru", marker='1')
		# plt.plot(window_size, custom_decay_slru[cache_size], label="decay custom slru", marker='2')
		# plt.plot(window_size, custom_decay_opt[cache_size], label="decay custom opt", marker='3')
		# plt.plot(window_size, wtinylfu_list[cache_size], label="wtinylfu", marker='v')
		# plt.xlabel("window lru size")
		# plt.ylabel("latency")
		# plt.yscale("log")
		# plt.title(item + " " + str(cache_size))
		# plt.legend()
		# plt.savefig("custom/result/"+catalog+granularity+item+"/"+str(cache_size)+".png")
		# plt.clf()

def plot_no_window(catalog, granularity, item, cache_size_list, custom_lru, custom_slru, custom_opt, wtinylfu_list):
	# for each file, plot no window latency v.s. cache size
	labels = ["no decay, count before", "no decay, count after", "decay, count before", "decay, count after"]
	markers = ["x", "^", "1", "2"]

	nw_wtinylfu = []
	for cache_size in cache_size_list:
		nw_wtinylfu.append(wtinylfu_list[cache_size][0])

	for i in range(4):	
		nw_lru = []
		# nw_slru = []
		# nw_opt = []
		for cache_size in cache_size_list:
			nw_lru.append(custom_lru[i][cache_size][0])
			# nw_slru.append(custom_slru[i][cache_size][0])
			# nw_opt.append(custom_opt[i][cache_size][0])
		plt.plot(cache_size_list, nw_lru, label="lru "+labels[i], marker = markers[i])
		# plt.plot(cache_size_list, nw_slru, label="slru "+labels[i])
		# plt.plot(cache_size_list, nw_opt, label="opt "+labels[i])
	plt.plot(cache_size_list, nw_wtinylfu, label="wtinylfu", marker="v")
	plt.xlabel("cache size")
	plt.ylabel("latency")
	plt.yscale("log")
	plt.title(item + " no window lru")
	plt.legend()
	plt.savefig("custom/result/"+catalog+granularity+item+"_lru.png")
	plt.close()

	for i in range(4):	
		nw_slru = []

		for cache_size in cache_size_list:
			nw_slru.append(custom_slru[i][cache_size][0])
		plt.plot(cache_size_list, nw_slru, label="slru "+labels[i], marker = markers[i])
		# plt.plot(cache_size_list, nw_opt, label="opt "+labels[i])
	plt.plot(cache_size_list, nw_wtinylfu, label="wtinylfu", marker="v")
	plt.xlabel("cache size")
	plt.ylabel("latency")
	plt.yscale("log")
	plt.title(item + " no window slru")
	plt.legend()
	plt.savefig("custom/result/"+catalog+granularity+item+"_slru.png")
	plt.close()

	for i in range(4):	
		nw_opt = []
		for cache_size in cache_size_list:
			nw_opt.append(custom_opt[i][cache_size][0])
		plt.plot(cache_size_list, nw_opt, label="opt "+labels[i], marker = markers[i])
	plt.plot(cache_size_list, nw_wtinylfu, label="wtinylfu", marker="v")
	plt.xlabel("cache size")
	plt.ylabel("latency")
	plt.yscale("log")
	plt.title(item + " no window opt")
	plt.legend()
	plt.savefig("custom/result/"+catalog+granularity+item+"_opt.png")
	plt.close()
		

def load():
	catalog = "short/"
	granularity = "max/"
	_dir = "custom/" + catalog + granularity

	# dirs = os.listdir(_dir)
	dirs = ["trace_fft", "trace_ocean_cp", "trace_ocean_ncp"]

	for item in dirs:
		cache_size_list = []
		window_list = {}
		
		# for each main cache type:
		# 0-5: no discount
		# 6-11: discount
		# 0-2, 6-8: count before window
		# 3-5, 9-11: count after window
		# 12: wtinylfu
		# 0: no discount, count first
		# 1: no discount, count later
		# 2: discount, count first
		# 3: discount, count later
		custom_lru = {}
		custom_slru = {}
		custom_opt = {}

		for i in range(4):
			custom_lru[i] = {}
			custom_slru[i] = {}
			custom_opt[i] = {}
		wtinylfu_list = {}
		
		files = os.listdir(_dir+item)

		for file in files:
			cache_size = int(file)
			cache_size_list.append(cache_size)
			f=open(_dir+item+"/"+file)
			line = f.readline()
			if line == "":
				continue
			window_size = np.array(line.split("\n")[0].split(" ")[0:-1]).astype(int)
			window_list[cache_size] = window_size

			for i in range(4):
				line = f.readline()
				lru = np.array(line.split("\n")[0].split(" ")[0:-1]).astype(int)
				custom_lru[i][cache_size] = lru

				line = f.readline()
				slru = np.array(line.split("\n")[0].split(" ")[0:-1]).astype(int)
				custom_slru[i][cache_size] = slru

				line = f.readline()
				opt = np.array(line.split("\n")[0].split(" ")[0:-1]).astype(int)
				custom_opt[i][cache_size] = opt

			line = f.readline()
			wtinylfu = np.array(line.split("\n")[0].split(" ")[0:-1]).astype(int)
			wtinylfu_list[cache_size] = wtinylfu

		cache_size_list = np.sort(np.asarray(cache_size_list).astype(int))
		plot(catalog, granularity, item, cache_size_list, window_list, custom_lru, custom_slru, custom_opt, wtinylfu_list)
		plot_no_window(catalog, granularity, item, cache_size_list, custom_lru, custom_slru, custom_opt, wtinylfu_list)

if __name__ == '__main__':
	load()