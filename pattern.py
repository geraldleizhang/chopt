import os
import sys
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
np.set_printoptions(suppress=True)
import multiprocessing as mp

from sklearn.linear_model import LinearRegression, LogisticRegression

sample_rates = {}

def get_sample_rate():
	length_file = open("trace_length")
	data = length_file.readlines()
	for line in data:
		_line = line.split("\n")[0].split(" ")
		if len(_line) == 1:
			continue
		_len = int(_line[-2])
		_file = _line[-1].split(".")[0]

		if _file not in sample_rates:
			sample_rates[_file] = _len
		else:
			#print(_file, sample_rates[_file], "&", _len)
			#sample_rates[_file] = _len * 100 / sample_rates[_file]
			#print("&", int(sample_rates[_file] * 100) / 100)
			sample_rates[_file] = sample_rates[_file] // _len 
			if "trace" in _file:
				sample_rates[_file] *= 64
			else:
				sample_rates[_file] *= 16
	# for fname in sample_rates:
	# 	print(fname, sample_rates[fname])


def load_unique_num():
	catalogs = ["parsec", "parsec2", "cp", "akamai"]
	for catalog in catalogs:
		unique_file = open("../data/unique/"+catalog)
		for line in unique_file:
			_l = line.split("\n")[0].split(" ")
			fname = _l[2].split(".")[0]
			length_list[fname] = int(_l[0])
			unique_num_list[fname] = int(_l[1])



catalog_group = {}

# files:
# original file 	../data/sampled/catalog
# read/write
# reuse distance 	../data/rd/catalog
# age 				../data/age/catalog
# frequency 		../data/frequency/catalog/cache_size
file_dir = "../data/sampled/"
rd_dir = "../data/rd/"
age_dir = "../data/age/"
freq_dir = "../data/frequency/"


# all others 	 		name:[data]
# freq 					name: {window_size:[freq]}
file = {}
rw = {}
rd = {}
age = {}
freq = {}

freq_variance = {}
freq_trending = {}
# filenames, without any succ
filenames = []
# sizes for freq 		name: [windows]
freq_sizes = {}


# simulation result
# filename, length, unique num, 
# "windows", windows, 
# "cache size", cache size, 
# for online: 
# 	"lru cost", "lfu cost", "tinylfu cost", "wtinylfu cost", "tinylfu only cost", "slru cost"
# for offline:
# 	"chopt cost", "belady cost", "static cost"
simulation_dir = "simulation/"
length_list = {}
windows_list = {}
unique_num_list = {}
cache_size_list = {}
chopt_cost_list = {}
belady_cost_list = {}
beladyac_cost_list = {}
static_cost_list = {}
lru_cost_list = {}
lfu_cost_list = {}
tinylfu_cost_list = {}
wtinylfu_cost_list = {}
tinylfu_only_cost_list = {}
slru_cost_list = {}

# cache result 		cache_behavior/catalog/granularity/cache_group
# offline: chopt, belady, static
# online: lru, lfu, tinylfu, W-tinylfu, tinylfu-only, slru
# cache result 		fname - cache_type - cache_size
result_dir = "cache_behavior/"
cache_result = {}
online_cache_types = ["lru", "lfu", "tinylfu", "W-tinylfu", "tinylfu-only", "slru"]
offline_cache_types = ["chopt", "belady", "beladyac"]

output_dir = "pattern_result/"

def per(x):
	return str(int(100000*x)/1000) + "\\% &"

def load_names():
	temp = os.listdir(file_dir + catalog)
	for item in temp:
		fname = item.split(".")[0]
		#if fname == "trace_canneal":
		#	continue
		filenames.append(fname)

def handle_load_pattern(catalog, fname, fullname):
	# file, rw
	_file = []
	_rw = []
	f = open(file_dir+catalog+fullname)
	lines = f.readlines()
	for line in lines:
		_l = line.split("\n")[0].split(",")
		if _l[0] == "page":
			continue
		if(catalog != "cp/"):
			_file.append(_l[0])
			_rw.append(1 if _l[2] == "w" else 0)
		elif(catalog == "cp/"):
			_file.append(_l[0])
			_rw.append(1 if _l[3] == "w" else 0)
	_rw = np.array(_rw).astype(int)
	f.close()

	# rd, age
	#_rd = np.loadtxt(rd_dir+catalog+fname).astype(int)
	_rd = np.asarray(pd.read_csv(rd_dir+catalog+fname, header=None, sep=" ")).astype(int)
	
	#_age = np.loadtxt(age_dir+catalog+fname).astype(int)
	_age = np.asarray(pd.read_csv(age_dir+catalog+fname, header=None, sep=" ")).astype(int)

	# freq
	_freq = {}
	windows = np.sort(np.array(os.listdir(freq_dir+catalog+fname)).astype(int))
	print(windows)
	for w in windows:
		#freq[w] = np.loadtxt(freq_dir+catalog+fname+"/"+str(w)).astype(int)		
		_freq[w] = np.asarray(pd.read_csv(freq_dir+catalog+fname+"/"+str(w), header=None, sep=" ")).astype(int)
	return _file, _rw, _rd, _age, windows, _freq
	# return _file

def load_pattern(catalog):
	catalog_group[catalog] = []
	temp = os.listdir(file_dir + catalog)

	pool = mp.Pool(processes = len(temp))
	res = {}
	fullnames = []
	_filenames = []
	for i in range(len(temp)):
		fname = temp[i].split(".")[0]
			
		#if fname != "lax_1448_6":
		#	continue	
		fullnames.append(temp[i])
		_filenames.append(fname)
		filenames.append(fname)
		catalog_group[catalog].append(fname)

	print(_filenames)

	for i in range(len(_filenames)):
		res[i] = pool.apply_async(handle_load_pattern, args=(catalog, _filenames[i], fullnames[i]))
		#res[i] = handle_load_pattern(catalog, filenames[i], fullnames[i])

	pool.close()
	pool.join()
	

	for i in range(len(_filenames)):
		fname = _filenames[i].split(".")[0]
		print(fname)
		_res = res[i].get()

		file[fname] = _res[0]
		rw[fname] = _res[1]
		rd[fname] = _res[2]
		age[fname] = _res[3]
		freq_sizes[fname] = _res[4]
		freq[fname] = _res[5]


def load_simulation_result(catalog, granularity, option):
	if len(filenames) == 0:
		load_names()

	for fname in filenames:	
		# if fname != "lax_1448_6":
		# 	continue
		if fname not in catalog_group[catalog]:
			continue
		file = open(simulation_dir+catalog+granularity+option+fname)
		line = file.readline().split("\n")[0].split(" ")
		length = int(line[3])
		length_list[fname] = length
		unique_num = int(line[6])
		unique_num_list[fname] = unique_num

		line = file.readline().split("\n")[0].split(" ")
		windows = np.sort(np.array(line[1:-1]).astype(int))
		windows_list[fname] = windows

		line = file.readline().split("\n")[0].split(" ")
		cache_sizes = np.sort(np.array(line[2:-1]).astype(int))
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

		elif option == "offline/":
			line = file.readline().split("\n")[0].split(" ")
			chopt_cost = np.array(line[2:-1]).astype(int)
			chopt_cost_list[fname] = chopt_cost

			line = file.readline().split("\n")[0].split(" ")
			belady_cost = np.array(line[2:-1]).astype(int)
			belady_cost_list[fname] = belady_cost

			line = file.readline().split("\n")[0].split(" ")
			beladyac_cost = np.array(line[2:-1]).astype(int)
			beladyac_cost_list[fname] = beladyac_cost

			# line = file.readline().split("\n")[0].split(" ")
			# static_cost = np.array(line[2:-1]).astype(int)
			# static_cost_list[fname] = static_cost
	

def handle_load_result(catalog, granularity, fname, option):
	cache_result = {}

	if option == 0:
		file = np.loadtxt(result_dir+catalog+granularity+"offline/"+fname).astype(int)
		for i in range(file.shape[0]):
			cache_type = int(i / (file.shape[0]/len(offline_cache_types)))
			if cache_type not in cache_result:
				cache_result[cache_type] = {}
			cache_size = file[i][0]
			cache_result[cache_type][cache_size] = file[i][1:]

	else:
		file = np.loadtxt(result_dir+catalog+granularity+"online/"+fname).astype(int)
		for i in range(file.shape[0]):
			cache_type = int(i / (file.shape[0]/len(online_cache_types)) + len(offline_cache_types))
			if cache_type not in cache_result:
				cache_result[cache_type] = {}
			cache_size = file[i][0]
			cache_result[cache_type][cache_size] = file[i][1:]

	return cache_result


# offline: chopt, belady, static
# online: lru, lfu, tinylfu, W-tinylfu, tinylfu-only, slru
# cache result 		fname - granularity - cache_type - cache_size
def load_result(catalog, granularity):
	#if len(filenames) == 0:
	#	load_names()

	# pool = mp.Pool(processes = len(filenames))
	# res = {}
	# for i in range(2*len(filenames)):
	# 	fname = filenames[int(i/2)]
	# 	res[i] = pool.apply_async(handle_load_result, args=(catalog, granularity, fname, i%2))

	# pool.close()
	# pool.join()

	# for i in range(2*len(filenames)):
	# 	fname = filenames[int(i/2)]
	# 	if fname not in cache_result:
	# 		cache_result[fname] = {}
	# 	temp =  res[i].get()
	# 	for item in temp:
	# 		cache_result[fname][item] = temp[item]


	for fname in filenames:
		# if fname != "lax_1448_6":
		# 	continue
		if fname not in catalog_group[catalog]:
			continue
		#print(fname)
		cache_result[fname] = {}
		#offline_file = np.genfromtxt(result_dir+catalog+granularity+"offline/"+fname, dtype=None).astype(int)
		offline_file = np.loadtxt(result_dir+catalog+granularity+"offline/"+fname).astype(int)
		#offline_file = np.asarray(pd.read_csv(result_dir+catalog+granularity+"offline/"+fname, sep=' ')).astype(int)
		for i in range(offline_file.shape[0]):
			#print("  ",i)
			cache_type = int(i / (offline_file.shape[0]/len(offline_cache_types)))
			if cache_type not in cache_result[fname]:
				cache_result[fname][cache_type] = {}
			cache_size = offline_file[i][0]
			cache_result[fname][cache_type][cache_size] = offline_file[i][1:]

		if(offline_file.shape[0] < len(cache_size_list[fname])):
			cache_size_list[fname] = cache_size_list[fname][:-1]

		# _file = open(result_dir+catalog+granularity+"offline/"+fname)
		# for i in range(len(offline_cache_types) * len(cache_size_list[fname])):
		# 	cache_type = i // len(cache_size_list[fname])
		# 	if cache_type not in cache_result[fname]:
		#  		cache_result[fname][cache_type] = {}
		# 	line = _file.readline().split("\n")[0].split(" ")[:-1]
		# 	cache_size = line[0]
		# 	cache_result[fname][cache_type][cache_size] = np.asarray(line[1:]).astype(int)
		# 	print(i, len(line[1:]), len(file[fname]))

		#online_file = np.genfromtxt(result_dir+catalog+granularity+"online/"+fname, dtype=None).astype(int)
		online_file = np.loadtxt(result_dir+catalog+granularity+"online/"+fname).astype(int)
		#online_file = np.asarray(pd.read_csv(result_dir+catalog+granularity+"online/"+fname, sep=' ')).astype(int)
		for i in range(online_file.shape[0]):
			#print("  ",i)
			cache_type = int(i / (online_file.shape[0]/len(online_cache_types)) + len(offline_cache_types))
			if cache_type not in cache_result[fname]:
				cache_result[fname][cache_type] = {}
			cache_size = online_file[i][0]
			cache_result[fname][cache_type][cache_size] = online_file[i][1:]

		print(fname + " result loaded")

	
def check_load_status(option):
	if option:
		print(len(filenames))
	if(len(filenames) == 0):
		print("filenames not loaded")
	if option:
		print(len(file))
	if(len(filenames) != len(file)):
		print("files not loaded")
	if option:
		print(len(rw))
	if(len(filenames) != len(rw)):
		print("rw not loaded")
	if option:
		print(len(rd))
	if(len(filenames) != len(rd)):
		print("rd not loaded")
	if option:
		print(len(age))
	if(len(filenames) != len(age)):
		print("age not loaded")
	if option:
		print(len(freq))
	if(len(filenames) != len(freq)):
		print("freq not loaded")
	
	for item in freq:
		if(len(freq[item]) != len(freq_sizes[item])):
			print("freq",item,"not loaded")

	for i in cache_result:
		if(len(cache_result[i][0])) == 0:
			print("cache result not loaded")
		for j in cache_result[i]:
			if option:
				for k in cache_result[i][j]:
					print("  ", k, len(cache_result[i][j][k]))


def plot_diff_all(fname, base, comp, output_file, cache_size):
	l = len(base)
	end = 0
	stat = []
	stat1 = []
	stat2 = []
	count = 0

	plt.figure(figsize=(20,10))
	# rd
	plt.subplot(2,3,1)
	x = np.arange(l)
	y = np.zeros(l)
	z = np.zeros(l)
	s = np.zeros(l)
	t = np.zeros(l)
	stat = rd[fname]
	for i in range(l):
		if (base[i] and not comp[i]):
			if stat[i] != -1:
				y[stat[i]] += 1
		elif (not base[i] and comp[i]):
			if stat[i] != -1:
				z[stat[i]] += 1
		elif (base[i] and comp[i]):
			if stat[i] != -1:
				s[stat[i]] += 1
		elif (not base[i] and not comp[i]):
			if stat[i] != -1:
				t[stat[i]] += 1

	for i in range(1,l):
		y[i] = y[i-1]+y[i]
		z[i] = z[i-1]+z[i]
		s[i] = s[i-1]+s[i]
		t[i] = t[i-1]+t[i]
		
	end1 = 0
	end2 = 0
	end3 = 0
	end4 = 0
	if y[-1] != 0:
		y = y / y[-1]
		end1 = np.argmax(y>=1)
	if z[-1] != 0:
		z = z / z[-1]
		end2 = np.argmax(z>=1)
	if s[-1] != 0:
		s = s / s[-1]
		end3 = np.argmax(s>=1)
	if t[-1] != 0:
		t = t / t[-1]
		end4 = np.argmax(t>=1)
	plt.plot(x[0:end1],y[0:end1], color='r', label="OPT only", marker='o')
	plt.plot(x[0:end2],z[0:end2], color='b', label="Comp only", marker='x')
	plt.plot(x[0:end3],s[0:end3], color='g', label="Both", marker='^')
	plt.plot(x[0:end4],t[0:end4], color='c', label="Neither", marker='v')
	plt.xlabel("rd")
	plt.ylabel("cdf")
	plt.legend()

	# age
	plt.subplot(2,3,2)
	plt.title(fname+" length: "+str(l)+" cache size: "+str(cache_size) + " " + str(granularity))
	x = np.arange(l)
	y = np.zeros(l)
	z = np.zeros(l)
	s = np.zeros(l)
	t = np.zeros(l)
	stat = age[fname]
	for i in range(l):
		if (base[i] and not comp[i]):
			if stat[i] != -1:
				y[stat[i]] += 1
		elif (not base[i] and comp[i]):
			if stat[i] != -1:
				z[stat[i]] += 1
		elif (base[i] and comp[i]):
			if stat[i] != -1:
				s[stat[i]] += 1
		elif (not base[i] and not comp[i]):
			if stat[i] != -1:
				t[stat[i]] += 1

	for i in range(1,l):
		y[i] = y[i-1]+y[i]
		z[i] = z[i-1]+z[i]
		s[i] = s[i-1]+s[i]
		t[i] = t[i-1]+t[i]
		
	end1 = 0
	end2 = 0
	end3 = 0
	end4 = 0
	if y[-1] != 0:
		y = y / y[-1]
		end1 = np.argmax(y>=1)
	if z[-1] != 0:
		z = z / z[-1]
		end2 = np.argmax(z>=1)
	if s[-1] != 0:
		s = s / s[-1]
		end3 = np.argmax(s>=1)
	if t[-1] != 0:
		t = t / t[-1]
		end4 = np.argmax(t>=1)
	plt.plot(x[0:end1],y[0:end1], color='r', label="OPT only", marker='o')
	plt.plot(x[0:end2],z[0:end2], color='b', label="Comp only", marker='x')
	plt.plot(x[0:end3],s[0:end3], color='g', label="Both", marker='^')
	plt.plot(x[0:end4],t[0:end4], color='c', label="Neither", marker='v')
	plt.xlabel("age")
	plt.ylabel("cdf")
	plt.legend()

	# freq
	plt.subplot(2,3,3)
	x = np.arange(l)
	y = np.zeros(l)
	z = np.zeros(l)
	s = np.zeros(l)
	t = np.zeros(l)
	stat = freq[fname][1000]
	
	for i in range(l):
		if (base[i] and not comp[i]):
			if stat[i] != -1:
				y[stat[i]] += 1
		elif (not base[i] and comp[i]):
			if stat[i] != -1:
				z[stat[i]] += 1
		elif (base[i] and comp[i]):
			if stat[i] != -1:
				s[stat[i]] += 1
		elif (not base[i] and not comp[i]):
			if stat[i] != -1:
				t[stat[i]] += 1

	for i in range(1,l):
		y[i] = y[i-1]+y[i]
		z[i] = z[i-1]+z[i]
		s[i] = s[i-1]+s[i]
		t[i] = t[i-1]+t[i]
		
	end1 = 0
	end2 = 0
	end3 = 0
	end4 = 0
	if y[-1] != 0:
		y = y / y[-1]
		end1 = np.argmax(y>=1)
	if z[-1] != 0:
		z = z / z[-1]
		end2 = np.argmax(z>=1)
	if s[-1] != 0:
		s = s / s[-1]
		end3 = np.argmax(s>=1)
	if t[-1] != 0:
		t = t / t[-1]
		end4 = np.argmax(t>=1)
	plt.plot(x[0:end1],y[0:end1], color='r', label="OPT only", marker='o')
	plt.plot(x[0:end2],z[0:end2], color='b', label="Comp only", marker='x')
	plt.plot(x[0:end3],s[0:end3], color='g', label="Both", marker='^')
	plt.plot(x[0:end4],t[0:end4], color='c', label="Neither", marker='v')
	plt.xlabel("freq")
	plt.ylabel("cdf")
	plt.legend()

	# rd-age
	plt.subplot(2,3,4)
	x = []
	y = []
	x_non = []
	y_non = []
	s = []
	t = []
	s_non = []
	t_non = []
	stat1 = rd[fname]
	stat2 = age[fname]
	plt.ylabel("age")
	for i in range(l):
		if (base[i] and not comp[i]):
			if stat1[i] != -1 and stat2[i] != -1:
				x.append(stat1[i])
				y.append(stat2[i])
		elif (not base[i] and comp[i]):
			if stat1[i] != -1 and stat2[i] != -1:
				x_non.append(stat1[i])
				y_non.append(stat2[i])
		elif (base[i] and comp[i]):
			if stat1[i] != -1 and stat2[i] != -1:
				s.append(stat1[i])
				t.append(stat2[i])
		elif (not base[i] and not comp[i]):
			if stat1[i] != -1 and stat2[i] != -1:
				s_non.append(stat1[i])
				t_non.append(stat2[i])
	plt.scatter(x,y, color='r', label="OPT only", alpha=0.05)
	plt.scatter(x_non,y_non, color='b', label="Comp only", alpha=0.05)
	plt.scatter(s,t, color='g', label="Both", alpha=0.05)
	plt.scatter(s_non,t_non, color='c', label="Neither", alpha=0.05)
	plt.xlabel("rd")
	plt.legend()

	# rd-freq
	plt.subplot(2,3,5)
	x = []
	y = []
	x_non = []
	y_non = []
	s = []
	t = []
	s_non = []
	t_non = []
	stat1 = rd[fname]
	
	stat2 = freq[fname][1000]
	plt.ylabel("freq")
	for i in range(l):
		if (base[i] and not comp[i]):
			if stat1[i] != -1 and stat2[i] != -1:
				x.append(stat1[i])
				y.append(stat2[i])
		elif (not base[i] and comp[i]):
			if stat1[i] != -1 and stat2[i] != -1:
				x_non.append(stat1[i])
				y_non.append(stat2[i])
		elif (base[i] and comp[i]):
			if stat1[i] != -1 and stat2[i] != -1:
				s.append(stat1[i])
				t.append(stat2[i])
		elif (not base[i] and not comp[i]):
			if stat1[i] != -1 and stat2[i] != -1:
				s_non.append(stat1[i])
				t_non.append(stat2[i])
	plt.scatter(x,y, color='r', label="OPT only", alpha=0.05)
	plt.scatter(x_non,y_non, color='b', label="Comp only", alpha=0.05)
	plt.scatter(s,t, color='g', label="Both", alpha=0.05)
	plt.scatter(s_non,t_non, color='c', label="Neither", alpha=0.05)
	plt.xlabel("rd")
	plt.legend()

	# age-freq
	plt.subplot(2,3,6)
	x = []
	y = []
	x_non = []
	y_non = []
	s = []
	t = []
	s_non = []
	t_non = []
	stat1 = age[fname]
	
	stat2 = freq[fname][1000]
	plt.ylabel("freq")
	for i in range(l):
		if (base[i] and not comp[i]):
			if stat1[i] != -1 and stat2[i] != -1:
				x.append(stat1[i])
				y.append(stat2[i])
		elif (not base[i] and comp[i]):
			if stat1[i] != -1 and stat2[i] != -1:
				x_non.append(stat1[i])
				y_non.append(stat2[i])
		elif (base[i] and comp[i]):
			if stat1[i] != -1 and stat2[i] != -1:
				s.append(stat1[i])
				t.append(stat2[i])
		elif (not base[i] and not comp[i]):
			if stat1[i] != -1 and stat2[i] != -1:
				s_non.append(stat1[i])
				t_non.append(stat2[i])
	plt.scatter(x,y, color='r', label="OPT only", alpha=0.05)
	plt.scatter(x_non,y_non, color='b', label="Comp only", alpha=0.05)
	plt.scatter(s,t, color='g', label="Both", alpha=0.05)
	plt.scatter(s_non,t_non, color='c', label="Neither", alpha=0.05)
	plt.xlabel("age")
	plt.legend()
	
	plt.savefig(output_file + fname + "_" + str(cache_size) + ".png")
	plt.close()

def calc_diff(fname, cache_size, base, comp):
	l = len(base)
	end = 0
	stat = []
	stat1 = []
	stat2 = []
	count = 0


	# rd
	x = np.arange(l)
	y = np.zeros(l)
	z = np.zeros(l)
	s = np.zeros(l)
	t = np.zeros(l)
	stat = rd[fname]
	for i in range(l):
		if (base[i] and not comp[i]):
			if stat[i] != -1:
				y[stat[i]] += 1
		elif (not base[i] and comp[i]):
			if stat[i] != -1:
				z[stat[i]] += 1
		elif (base[i] and comp[i]):
			if stat[i] != -1:
				s[stat[i]] += 1
		elif (not base[i] and not comp[i]):
			if stat[i] != -1:
				t[stat[i]] += 1

	for i in range(1,l):
		y[i] = y[i-1]+y[i]
		z[i] = z[i-1]+z[i]
		s[i] = s[i-1]+s[i]
		t[i] = t[i-1]+t[i]
		
	end1 = 0
	end2 = 0
	end3 = 0
	end4 = 0
	if y[-1] != 0:
		y = y / y[-1]
		end1 = np.argmax(y>=1)
	if z[-1] != 0:
		z = z / z[-1]
		end2 = np.argmax(z>=1)
	if s[-1] != 0:
		s = s / s[-1]
		end3 = np.argmax(s>=1)
	if t[-1] != 0:
		t = t / t[-1]
		end4 = np.argmax(t>=1)


	# age
	x = np.arange(l)
	y = np.zeros(l)
	z = np.zeros(l)
	s = np.zeros(l)
	t = np.zeros(l)
	stat = age[fname]
	for i in range(l):
		if (base[i] and not comp[i]):
			if stat[i] != -1:
				y[stat[i]] += 1
		elif (not base[i] and comp[i]):
			if stat[i] != -1:
				z[stat[i]] += 1
		elif (base[i] and comp[i]):
			if stat[i] != -1:
				s[stat[i]] += 1
		elif (not base[i] and not comp[i]):
			if stat[i] != -1:
				t[stat[i]] += 1

	for i in range(1,l):
		y[i] = y[i-1]+y[i]
		z[i] = z[i-1]+z[i]
		s[i] = s[i-1]+s[i]
		t[i] = t[i-1]+t[i]
		
	end1 = 0
	end2 = 0
	end3 = 0
	end4 = 0
	if y[-1] != 0:
		y = y / y[-1]
		end1 = np.argmax(y>=1)
	if z[-1] != 0:
		z = z / z[-1]
		end2 = np.argmax(z>=1)
	if s[-1] != 0:
		s = s / s[-1]
		end3 = np.argmax(s>=1)
	if t[-1] != 0:
		t = t / t[-1]
		end4 = np.argmax(t>=1)


	# freq
	x = np.arange(l)
	y = np.zeros(l)
	z = np.zeros(l)
	s = np.zeros(l)
	t = np.zeros(l)
	stat = freq[fname][1000]
	
	for i in range(l):
		if (base[i] and not comp[i]):
			if stat[i] != -1:
				y[stat[i]] += 1
		elif (not base[i] and comp[i]):
			if stat[i] != -1:
				z[stat[i]] += 1
		elif (base[i] and comp[i]):
			if stat[i] != -1:
				s[stat[i]] += 1
		elif (not base[i] and not comp[i]):
			if stat[i] != -1:
				t[stat[i]] += 1

	for i in range(1,l):
		y[i] = y[i-1]+y[i]
		z[i] = z[i-1]+z[i]
		s[i] = s[i-1]+s[i]
		t[i] = t[i-1]+t[i]
		
	end1 = 0
	end2 = 0
	end3 = 0
	end4 = 0
	if y[-1] != 0:
		y = y / y[-1]
		end1 = np.argmax(y>=1)
	if z[-1] != 0:
		z = z / z[-1]
		end2 = np.argmax(z>=1)
	if s[-1] != 0:
		s = s / s[-1]
		end3 = np.argmax(s>=1)
	if t[-1] != 0:
		t = t / t[-1]
		end4 = np.argmax(t>=1)

	

def plot_opt_all(fname, base, output_file, cache_size):
	l = len(base)
	end = 0
	stat = []
	stat1 = []
	stat2 = []
	count = 0

	plt.figure(figsize=(20,10))
	# rd
	plt.subplot(2,3,1)
	count = 0
	x = np.arange(l)
	y = np.zeros(l)
	z = np.zeros(l)
	stat = rd[fname]
	for i in range(l):
		if base[i]:
			if stat[i] != -1:
				y[stat[i]] += 1
				count+=1
		else:
			if stat[i] != -1:
				z[stat[i]] += 1

	for i in range(1,l):
		y[i] = y[i-1]+y[i]
		z[i] = z[i-1]+z[i]
	end1 = 0
	end2 = 0
	if y[-1] != 0:
		y = y / y[-1]
		end1 = np.argmax(y>=1)
	if z[-1] != 0:
		z = z / z[-1]
		end2 = np.argmax(z>=1)
	plt.plot(x[0:end1],y[0:end1], color='r', label="cached", marker='o')
	plt.plot(x[0:end2],z[0:end2], color='b', label="non-cached", marker='x')
	plt.xlabel("rd " + str(count))
	plt.ylabel("cdf")
	plt.legend()

	# age
	plt.subplot(2,3,2)
	count = 0
	plt.title(fname+" length: "+str(l)+" cache size: "+str(cache_size) + " " +str(granularity))
	x = np.arange(l)
	y = np.zeros(l)
	z = np.zeros(l)
	stat = age[fname]
	for i in range(l):
		if base[i]:
			if stat[i] != -1:
				y[stat[i]] += 1
				count+=1
		else:
			if stat[i] != -1:
				z[stat[i]] += 1

	for i in range(1,l):
		y[i] = y[i-1]+y[i]
		z[i] = z[i-1]+z[i]
	end1 = 0
	end2 = 0
	if y[-1] != 0:
		y = y / y[-1]
		end1 = np.argmax(y>=1)
	if z[-1] != 0:
		z = z / z[-1]
		end2 = np.argmax(z>=1)
	plt.plot(x[0:end1],y[0:end1], color='r', label="cached", marker='o')
	plt.plot(x[0:end2],z[0:end2], color='b', label="non-cached", marker='x')
	plt.xlabel("age " + str(count))
	plt.ylabel("cdf")
	plt.legend()

	# freq
	plt.subplot(2,3,3)
	count = 0
	x = np.arange(l)
	y = np.zeros(l)
	z = np.zeros(l)
	stat = freq[fname][1000]
	
	for i in range(l):
		if base[i]:
			if stat[i] != -1:
				y[stat[i]] += 1
				count+=1
		else:
			if stat[i] != -1:
				z[stat[i]] += 1

	for i in range(1,l):
		y[i] = y[i-1]+y[i]
		z[i] = z[i-1]+z[i]
	end1 = 0
	end2 = 0
	if y[-1] != 0:
		y = y / y[-1]
		end1 = np.argmax(y>=1)
	if z[-1] != 0:
		z = z / z[-1]
		end2 = np.argmax(z>=1)
	plt.plot(x[0:end1],y[0:end1], color='r', label="cached", marker='o')
	plt.plot(x[0:end2],z[0:end2], color='b', label="non-cached", marker='x')
	plt.xlabel("freq " + str(count))
	plt.ylabel("cdf")
	plt.legend()

	# rd-age
	plt.subplot(2,3,4)
	count = 0
	x = []
	y = []
	x_non = []
	y_non = []
	stat1 = rd[fname]
	
	stat2 = age[fname]
	plt.ylabel("age")
	for i in range(l):
		if base[i]:
			if stat1[i] != -1 and stat2[i] != -1:
				x.append(stat1[i])
				y.append(stat2[i])
				count+=1
		else:
			if stat1[i] != -1 and stat2[i] != -1:
				x_non.append(stat1[i])
				y_non.append(stat2[i])
	plt.scatter(x,y, color='r', label="cached", alpha=0.05)
	plt.scatter(x_non,y_non, color='b', label="non-cached", alpha=0.05)
	plt.xlabel("rd " + str(count))
	plt.legend()

	# rd-freq
	plt.subplot(2,3,5)
	count = 0
	x = []
	y = []
	x_non = []
	y_non = []
	stat1 = rd[fname]
	
	stat2 = freq[fname][1000]
	plt.ylabel("freq")
	for i in range(l):
		if base[i]:
			if stat1[i] != -1 and stat2[i] != -1:
				x.append(stat1[i])
				y.append(stat2[i])
				count+=1
		else:
			if stat1[i] != -1 and stat2[i] != -1:
				x_non.append(stat1[i])
				y_non.append(stat2[i])
	plt.scatter(x,y, color='r', label="cached", alpha=0.05)
	plt.scatter(x_non,y_non, color='b', label="non-cached", alpha=0.05)
	plt.xlabel("rd " + str(count))
	plt.legend()

	# age-freq
	plt.subplot(2,3,6)
	count = 0
	x = []
	y = []
	x_non = []
	y_non = []
	stat1 = age[fname]
	
	stat2 = freq[fname][1000]
	plt.ylabel("freq")
	for i in range(l):
		if base[i]:
			if stat1[i] != -1 and stat2[i] != -1:
				x.append(stat1[i])
				y.append(stat2[i])
				count+=1
		else:
			if stat1[i] != -1 and stat2[i] != -1:
				x_non.append(stat1[i])
				y_non.append(stat2[i])
	plt.scatter(x,y, color='r', label="cached", alpha=0.05)
	plt.scatter(x_non,y_non, color='b', label="non-cached", alpha=0.05)
	plt.xlabel("age " + str(count))
	plt.legend()
	
	plt.savefig(output_file + fname + "_" + str(cache_size) + ".png")
	plt.close()



# for a cache type, print patterns for different results from CHOPT(0)
def print_difference(catalog, granularity, cache_type):
	for fname in filenames:
		cache_sizes = np.sort(cache_size_list[fname])

		#if not os.path.exists(output_dir+catalog+granularity):
		#	os.makedirs(output_dir+catalog+granularity)

		for cache_size in cache_sizes:
			if not os.path.exists(output_dir+catalog+granularity + "diff/"):
				os.makedirs(output_dir+catalog+granularity + "diff/")
			
			if not os.path.exists(output_dir+catalog+granularity + "opt/"):
				os.makedirs(output_dir+catalog+granularity + "opt/")
			
		pool = mp.Pool(processes = len(cache_sizes)*2)
		res = {}
		for i in range(len(cache_sizes)*2):
			cache_size = cache_sizes[int(i/2)]
			option = int(i%2)
			if cache_size not in cache_result[fname][0]:
				continue
			if cache_size not in cache_result[fname][cache_type]:
				continue
				
			base = cache_result[fname][0][cache_size]
			comp = cache_result[fname][cache_type][cache_size]
			
			if not option:
				figure_dir = output_dir+catalog+granularity+ "diff/"
				res[i] = pool.apply_async(plot_diff_all, args=(fname, base, comp, figure_dir, cache_size))
				
			else:
				figure_dir = output_dir+catalog+granularity+ "opt/"
				res[i] = pool.apply_async(plot_opt_all, args=(fname, base, figure_dir, cache_size))
				
		pool.close()
		pool.join()

# for 1d np.array, calculate cdf array
def get_cdf(data):
	data = np.asarray(data)

	for i in range(data.shape[0] - 1):
		data[i+1] += data[i]

	return data/data[-1]


def calc_frequency():
	for fname in freq:
		if fname not in freq_trending:
			freq_variance[fname] = np.zeros((len(freq[fname][1000]),10))
			freq_trending[fname] = np.zeros((len(freq[fname][1000]),10))
		for window in  freq_sizes[fname]:
			_index = window // 1000 - 1
			if _index == 9:
				freq_variance[fname][:,_index] = np.transpose(freq[fname][window])
				indics = np.where(freq[fname][window] > 0)
				freq_trending[fname][indics[0],_index] = 1
				indics = np.where(freq[fname][window] == 0)
				freq_trending[fname][indics[0],_index] = 0
				indics = np.where(freq[fname][window] < 0)
				freq_trending[fname][indics[0],_index] = -1
			else:
				freq_variance[fname][:,_index] = np.transpose(freq[fname][window] - freq[fname][window+1000])
				indics = np.where(freq_variance[fname][:,_index] > 0)
				freq_trending[fname][indics[0],_index] = 1
				indics = np.where(freq_variance[fname][:,_index] == 0)
				freq_trending[fname][indics[0],_index] = 0
				indics = np.where(freq_variance[fname][:,_index] < 0)
				freq_trending[fname][indics[0],_index] = -1


	for fname in freq_trending:
		linear_regressor = LinearRegression(fit_intercept=True, normalize=True, copy_X=True)
		for cache_size in cache_result[fname][0]:
			X = freq_trending[fname][:,5:10] / 10
			Y = cache_result[fname][0][cache_size]
			Y = np.where(Y>1,1,0)
			lr_model = linear_regressor.fit(X,Y)
			pred = np.around(lr_model.predict(X))
			pred = np.where(pred>0.5, 1, 0)
			print(cache_size, np.count_nonzero(pred-Y), Y.shape, lr_model.coef_, lr_model.intercept_)

		# logistic_regressor = LogisticRegression()
		# for cache_size in cache_result[fname][0]:
		# 	X = freq_trending[fname][:,5:10] / 10
		# 	Y = cache_result[fname][0][cache_size]
		# 	Y = np.where(Y>1,1,0)
		# 	lg_model = logistic_regressor.fit(X,Y)
		# 	pred = lg_model.predict(X)
		# 	pred = np.where(pred>0.5, 1, 0)
		# 	print(cache_size, np.count_nonzero(pred-Y), Y.shape[0], lg_model.coef_, lg_model.intercept_)

# catalog:  	data type
# granularity:	sampled type
# cache_group: 	online/offline

def dynamic():
	for fname in filenames:
		cache_sizes = np.sort(cache_size_list[fname])
		_file = np.asarray(file[fname])
		_rd = rd[fname]
		_age = age[fname]
		_freq = freq[fname]
		for cache_size in cache_sizes:
			print(cache_size)
			res = cache_result[fname][0][cache_size]
			swap_in = _rd[np.where(res==2)[0]]
			swap_out = _rd[np.where(res==3)[0]]
			cached = _rd[np.where(res==1)[0]]
			non_cached = _rd[np.where(res==0)[0]]

			# handling function

			print(np.amin(swap_in), np.amax(swap_in), np.median(swap_in), np.average(swap_in), np.mean(swap_in))
			print(np.amin(swap_out), np.amax(swap_out), np.median(swap_out), np.average(swap_out), np.mean(swap_out))
			print(np.amin(cached), np.amax(cached), np.median(cached), np.average(cached), np.mean(cached))
			print(np.amin(non_cached), np.amax(non_cached), np.median(non_cached), np.average(non_cached), np.mean(non_cached))


def first_access(fname, cache_size):
	print("    [first access]")
	res = cache_result[fname][0][cache_size]
	_file = np.asarray(file[fname])
	_rd = rd[fname]
	# unique, counts = np.unique(res, return_counts = True)
	# print(res.shape, dict(zip(unique, counts)))
	
	first_occur_index = np.where(_rd == -1)[0]
	unique, counts = np.unique(_file, return_counts = True)
	print("    ", np.count_nonzero(res[first_occur_index]), len(unique))

def rd_distribution(fname, cache_size):
	print("    [reuse distance distribution]")
	res = cache_result[fname][0][cache_size]
	_file = np.asarray(file[fname])
	_rd = rd[fname]

	cached = np.where(res > 0)
	non_cached = np.where(res == 0)

	unique, counts = np.unique(_file, return_counts = True)
	print("    ",len(unique))

	unique, counts = np.unique(_rd[cached], return_counts = True)
	print("    ",len( np.where(counts > 10)[0] ))
	#print(counts)

	unique, counts = np.unique(_rd[non_cached], return_counts = True)
	print("    ",len( np.where(counts > 10)[0] ))
	#print(counts)

	cdf = get_cdf(counts)
	print("    ",unique, cdf)

decision_type = ["swap-in", "swap-out", "cached", "non-cached"]
# for chopt result, get decision type
def get_chopt_decision(base):
	return [np.where(base==2)[0], np.where(base==3)[0], np.where(base==1)[0], np.where(base==0)[0]]

# for other result, get decision type
def get_other_decision(base):
	temp = np.zeros(base.shape[0])
	temp[0] = base[0]
	temp[1:] = base[1:] - base[:-1]
	return [np.where(temp==1)[0], np.where(temp==-1)[0], np.where(np.where(base==1,1,0)*np.where(temp==0,1,0)==1)[0], np.where(base==0)[0]]

diff_type = ["both","none","opt-only","lru-only"]
# for chopt and lru result, get diff type
def get_diff(base, comp):
	base[base>0]=1
	return [np.where(np.where(base==1,1,0)*np.where(comp==1,1,0))[0], np.where(np.where(base==0,1,0)*np.where(comp==0,1,0))[0], np.where(np.where(base==1,1,0)*np.where(comp==0,1,0))[0], np.where(np.where(base==0,1,0)*np.where(comp==1,1,0))[0] ]

def dynamic_analysis():
	for fname in filenames:
		cache_sizes = np.sort(cache_size_list[fname])
		for cache_size in cache_sizes:
			print(fname, cache_size)
			_rd = rd[fname]
			chopt_res = cache_result[fname][0][cache_size]
			lru_res = cache_result[fname][3][cache_size]

			chopt_decision = get_chopt_decision(chopt_res)
			lru_decision = get_other_decision(lru_res)
			
			print("  stat on single decision")
			for i in range(4):
				_res = chopt_decision[i]
				if _res.shape[0]:
					print("    ", decision_type[i], np.amin(_rd[_res]), np.amax(_rd[_res]), np.median(_rd[_res]), np.mean(_rd[_res]), np.var(_rd[_res]))
				_res = lru_decision[i]
				if _res.shape[0]:
					print("    ", decision_type[i], np.amin(_rd[_res]), np.amax(_rd[_res]), np.median(_rd[_res]), np.mean(_rd[_res]), np.var(_rd[_res]))

			diff_res = get_diff(chopt_res, lru_res)
			print("  stat on different decisions")
			for i in range(4):
				_res = diff_res[i]
				if _res.shape[0]:
					print("    ", diff_type[i], np.amin(_rd[_res]), np.amax(_rd[_res]), np.median(_rd[_res]), np.mean(_rd[_res]), np.var(_rd[_res]))



			


			#first_access(fname, cache_size)
			#rd_distribution(fname, cache_size)

def compare_decision(fname, cache_size):
	if cache_size not in cache_result[fname][0]:
		return
	if cache_size not in cache_result[fname][1]:
		return
	print("    [different decisions]",fname, cache_size)

	chopt_res = cache_result[fname][0][cache_size]
	chopt_res = np.where(chopt_res > 0, 1, 0)
	belady_res = cache_result[fname][1][cache_size]
	print("    ",np.count_nonzero(chopt_res-belady_res),"/",chopt_res.shape[0])

#for chopt, items not 0 means hit
#for all others, only 1 means hit

def compare_hit_rate(fname, cache_size):
	hit_rate_log.write(fname + " " + str(cache_size) + "\n")
	if cache_size not in cache_result[fname][0]:
		#return 0,0,0,0,0
		return
	if cache_size not in cache_result[fname][1]:
		#return 0,0,1,0,0
		return
	if cache_size not in cache_result[fname][2]:
		#return 0,0,1,0,0
		return
	if cache_size not in cache_result[fname][3]:
		#return 0,0,3,0,0
		return
	if cache_size not in cache_result[fname][5]:
		#return 0,0,5,0,0
		return
	if cache_size not in cache_result[fname][6]:
		#return 0,0,6,0,0
		return
	#print("    [hit rate]",fname, cache_size)

	chopt_res = cache_result[fname][0][cache_size]
	chopt_res = np.where(chopt_res % 2 == 1, 1, 0)
	belady_res = cache_result[fname][1][cache_size]
	belady_res = np.where(belady_res == 1, 1, 0)
	beladyac_res = cache_result[fname][2][cache_size]
	beladyac_res = np.where(beladyac_res == 1, 1, 0)
	lru_res = cache_result[fname][3][cache_size]
	lru_res = np.where(lru_res == 1, 1, 0)
	tinylfu_res = cache_result[fname][5][cache_size]
	tinylfu_res = np.where(tinylfu_res == 1, 1, 0)
	wtinylfu_res = cache_result[fname][6][cache_size]
	wtinylfu_res = np.where(wtinylfu_res == 1, 1, 0)
	
	#print("    chopt : ",np.count_nonzero(chopt_res)/chopt_res.shape[0], "    ", 1-np.count_nonzero(chopt_res)/chopt_res.shape[0])
	#print("    belady: ",np.count_nonzero(belady_res)/chopt_res.shape[0], "    ", 1-np.count_nonzero(belady_res)/chopt_res.shape[0])
	#print("    lru   : ",np.count_nonzero(lru_res)/chopt_res.shape[0], "    ", 1-np.count_nonzero(lru_res)/chopt_res.shape[0])

	#return np.count_nonzero(chopt_res)/chopt_res.shape[0], np.count_nonzero(belady_res)/chopt_res.shape[0], np.count_nonzero(lru_res)/chopt_res.shape[0], np.count_nonzero(tinylfu_res)/chopt_res.shape[0], np.count_nonzero(wtinylfu_res)/chopt_res.shape[0]
	gran = 100000
	c_rate = np.count_nonzero(chopt_res)/chopt_res.shape[0]
	b_rate = np.count_nonzero(belady_res)/chopt_res.shape[0]
	bac_rate = np.count_nonzero(beladyac_res)/chopt_res.shape[0]
	l_rate = np.count_nonzero(lru_res)/chopt_res.shape[0]
	t_rate = np.count_nonzero(tinylfu_res)/chopt_res.shape[0]
	w_rate = np.count_nonzero(wtinylfu_res)/chopt_res.shape[0]
	#print("    ", int(c_rate*gran)/gran, int(b_rate*gran)/gran, int(l_rate*gran)/gran, int(t_rate*gran)/gran, int(w_rate*gran)/gran)
	hit_rate_log.write("    " + str(int(c_rate*gran)/gran) + " " + str(int(b_rate*gran)/gran) + " " + str(int(bac_rate*gran)/gran) + " " + str(int(l_rate*gran)/gran) + " " + str(int(t_rate*gran)/gran) + " " + str(int(w_rate*gran)/gran) + " " + str(np.count_nonzero(beladyac_res)-np.count_nonzero(belady_res)) + " " + str(np.count_nonzero(beladyac_res-belady_res)) + "\n")
	hit_rate_log.flush()


def comparison_analysis():
	for fname in filenames:
		cache_sizes = np.sort(cache_size_list[fname])
		for cache_size in cache_sizes:
			
			#compare_decision(fname, cache_size)
			compare_hit_rate(fname, cache_size)


def plot_hit_rate(isSamerw):
	g_count_1 = 0
	g_count_2 = 0
	get_sample_rate()
	#"pattern_result/parsec/sampled_parsec/",
	sources = ["pattern_result/parsec/sampled_parsec/", "pattern_result/cp/cp/", "pattern_result/akamai/akamai/"]
	if isSamerw:
		sources = ["pattern_result/parsec/samerw/", "pattern_result/cp/samerw/", "pattern_result/akamai/samerw/"]

	for source in sources:
		if not os.path.exists(source + "hit_rate_result/"):
			os.makedirs(source + "hit_rate_result/")
		workloads = []
		cache_sizes = {}
		hit_rates = {}
		lines  = open(source + "hit_rate.log", "r").readlines()
		count = 0
		# if source == "pattern_result/parsec/sampled_parsec/" or source == "pattern_result/parsec/samerw/":
		# 	count = -1
		trace_name = ""
		trace_size = 0
		for line in lines:
			# print(line, count)
			# if count <= 0:
			# 	_l = line.split("]")[0].split("[")[1].split(",")
			# 	for temp in _l:
			# 		workloads.append(temp.split("'")[1])
			# 		hit_rates[temp.split("'")[1]] = []
			# 		cache_sizes[temp.split("'")[1]] = []
			# 	count+=1
			# 	continue
			# elif count < 4:
			# 	count += 1
			# 	continue

			_l = line.split("\n")[0].split(" ")
			#print(_l)
			if _l[0] != "":
				trace_name = _l[0]
				trace_size = int(_l[1])
				#print(trace_name, trace_size)
				
			else:
				g_count_1+=1
				#if float(_l[7])-float(_l[9]) > 0.005:
				#	continue

				#if float(_l[6]) - float(_l[5]) < 0.00002:
				#	continue
				#print(trace_size, unique_num_list[trace_name])
				
				if "parsec" in source:
					if trace_size > unique_num_list[trace_name]*0.015:
						continue
				elif "cp" in source:
					if trace_size > unique_num_list[trace_name]*0.01:
						continue
				elif "akamai" in source:
					if trace_size > unique_num_list[trace_name]*0.001:
						continue

				#print(trace_name, trace_size)
				g_count_2+=1
				if trace_name not in workloads:
					workloads.append(trace_name)
					hit_rates[trace_name] = []
					cache_sizes[trace_name] = []
				rates = []
				rates.append(float(_l[4]))
				rates.append(float(_l[5]))
				rates.append(float(_l[6]))
				rates.append(float(_l[7]))
				rates.append(float(_l[8]))
				rates.append(float(_l[9]))

				cache_sizes[trace_name].append(trace_size)
				hit_rates[trace_name].append(rates)
				#count+=1

		total_rates = np.zeros(6)

		for trace in workloads:
			rates = np.asarray(hit_rates[trace])
			#print(trace, cache_sizes[trace], rates.shape)
			# plt.figure()
			# x= np.asarray(cache_sizes[trace]).astype(int) * sample_rates[trace]
			#print(trace, x, rates.shape)
			# algos = ["CHOPT", "Belady", "BeladyAD", "LRU", "TinyLFU", "W-TinyLFU"]
			# markers = ["o", "x", "s", "v", "*", "^"]
			for i in range(6):
				#plt.plot(x, 1-rates[:,i], label=algos[i], marker=markers[i])
				total_rates[i] += 1-rates[rates.shape[0]//2,i]
			# plt.legend()
			# plt.xlabel("cache size")
			# plt.ylabel("miss rate")
			# #plt.savefig(source+"hit_rate_result/"+trace+".png")
			# plt.close()
		
		res = 1-total_rates/len(workloads)
		stat.write(source+"\n")
		for _r in res:
			stat.write(per(_r) + " ")
		stat.write("\n")
		# print(source, 1-total_rates/len(workloads))
		# for i in range(4):
		# 	if i == 0:
		# 		print(total_rates[0]/total_rates[i+1]-1)
		# 	else:
		# 		print(1-total_rates[0]/total_rates[i+1])
	print(g_count_1, g_count_2)

if __name__ == '__main__':
	workload = sys.argv[1]

	isAnal = False

	# if len(sys.argv) >= 3:
	# 	if sys.argv[2] == "anal":
	# 		isAnal = True
	# else:
	# 	sys.exit()


	# if workload == "parsec":

	# 	load_pattern("parsec/")
	# 	load_pattern("parsec2/")
	# 	for fname in filenames:
	# 		_rd = rd[fname]
	# 		temp = np.where(_rd==-1)[0]
	# 		_rd[temp] = 0
	# 		print(fname, _rd.shape[0], temp.shape[0], np.sum(_rd)/(_rd.shape[0]-temp.shape[0]))


	# elif workload == "cp":
	# 	catalog = "cp/"
	# 	granularity = "cp/"
	# 	load_pattern(catalog)
	# 	for fname in filenames:
	# 		_rd = rd[fname]
	# 		temp = np.where(_rd==-1)[0]
	# 		_rd[temp] = 0
	# 		print(fname, _rd.shape[0], temp.shape[0], np.sum(_rd)/(_rd.shape[0]-temp.shape[0]))

	# elif workload == "akamai":
	# 	catalog = "akamai/"
	# 	granularity = "akamai/"
	# 	load_pattern(catalog)
	# 	for fname in filenames:
	# 		_rd = rd[fname]
	# 		temp = np.where(_rd==-1)[0]
	# 		_rd[temp] = 0
	# 		print(fname, _rd.shape[0], temp.shape[0], np.sum(_rd)/(_rd.shape[0]-temp.shape[0]))


	if workload == "parsec":
		load_pattern("parsec/")
		load_pattern("parsec2/")
		print("pattern loaded")

		catalog = "parsec/"
		granularity = "sampled_parsec/"
		load_simulation_result(catalog, granularity, "online/")
		load_simulation_result(catalog, granularity, "offline/")
		catalog = "parsec2/"
		load_simulation_result(catalog, granularity, "online/")
		load_simulation_result(catalog, granularity, "offline/")
		print("simulation loaded")

		catalog = "parsec/"
		load_result(catalog, granularity)		
		catalog = "parsec2/"
		load_result(catalog, granularity)
		print("result loaded")

		catalog = "parsec/"

		#dynamic_analysis()
		if isAnal:
			dynamic_analysis()
		else:	
			miss_rate_log_dir = catalog + granularity + "hit_rate.log"
			hit_rate_log = open(output_dir+miss_rate_log_dir, "w")
			comparison_analysis()

	elif workload == "parsec_samerw":
		load_pattern("parsec/")
		load_pattern("parsec2/")
		print("pattern loaded")

		catalog = "parsec/"
		granularity = "samerw/"
		load_simulation_result(catalog, granularity, "online/")
		load_simulation_result(catalog, granularity, "offline/")
		catalog = "parsec2/"
		load_simulation_result(catalog, granularity, "online/")
		load_simulation_result(catalog, granularity, "offline/")
		print("simulation loaded")

		catalog = "parsec/"
		load_result(catalog, granularity)		
		catalog = "parsec2/"
		load_result(catalog, granularity)
		print("result loaded")

		#dynamic_analysis()
		catalog = "parsec/"
		if isAnal:
			dynamic_analysis()
		else:	
			miss_rate_log_dir = catalog + granularity + "hit_rate.log"
			hit_rate_log = open(output_dir+miss_rate_log_dir, "w")
			comparison_analysis()


	elif workload == "cp":
		catalog = "cp/"
		granularity = "cp/"
		load_pattern(catalog)
		print("pattern loaded")
		load_simulation_result(catalog, granularity, "online/")
		load_simulation_result(catalog, granularity, "offline/")
		print("simulation loaded")
		load_result(catalog, granularity)
		print("result loaded")
		if isAnal:
			dynamic_analysis()
		else:	
			miss_rate_log_dir = catalog + granularity + "hit_rate.log"
			hit_rate_log = open(output_dir+miss_rate_log_dir, "w")
			comparison_analysis()

	elif workload == "cp_samerw":
		catalog = "cp/"
		granularity = "samerw/"
		load_pattern(catalog)
		print("pattern loaded")
		load_simulation_result(catalog, granularity, "online/")
		load_simulation_result(catalog, granularity, "offline/")
		print("simulation loaded")
		load_result(catalog, granularity)
		print("result loaded")
		if isAnal:
			dynamic_analysis()
		else:	
			miss_rate_log_dir = catalog + granularity + "hit_rate.log"
			hit_rate_log = open(output_dir+miss_rate_log_dir, "w")
			comparison_analysis()

	elif workload == "akamai":
		catalog = "akamai/"
		granularity = "akamai/"
		load_pattern(catalog)
		print("pattern loaded")
		load_simulation_result(catalog, granularity, "online/")
		load_simulation_result(catalog, granularity, "offline/")
		print("simulation loaded")
		load_result(catalog, granularity)
		print("result loaded")
		if isAnal:
			dynamic_analysis()
		else:	
			miss_rate_log_dir = catalog + granularity + "hit_rate.log"
			hit_rate_log = open(output_dir+miss_rate_log_dir, "w")
			comparison_analysis()

	elif workload == "akamai_samerw":
		catalog = "akamai/"
		granularity = "samerw/"
		load_pattern(catalog)
		print("pattern loaded")
		load_simulation_result(catalog, granularity, "online/")
		load_simulation_result(catalog, granularity, "offline/")
		print("simulation loaded")
		load_result(catalog, granularity)
		print("result loaded")
		if isAnal:
			dynamic_analysis()
		else:	
			miss_rate_log_dir = catalog + granularity + "hit_rate.log"
			hit_rate_log = open(output_dir+miss_rate_log_dir, "w")
			comparison_analysis()

	elif workload == "test":
		catalog = "test/"
		granularity = "test/"
		load_pattern(catalog)
		print("pattern loaded")
		load_simulation_result(catalog, granularity, "online/")
		load_simulation_result(catalog, granularity, "offline/")
		print("simulation loaded")
		load_result(catalog, granularity)
		print("result loaded")
		if isAnal:
			dynamic_analysis()
		else:	
			miss_rate_log_dir = catalog + granularity + "hit_rate.log"
			hit_rate_log = open(output_dir+miss_rate_log_dir, "w")
			comparison_analysis()



	elif workload == "hit_rate":
		stat = open("table_result/hit_rate.table", "w")
		load_unique_num()
		plot_hit_rate(0)
		plot_hit_rate(1)
			
	

	
	#first_access()
	#rd_distribution()
	#calc_frequency()
	#dynamic_analysis()
	#comparison_analysis()