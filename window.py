import os
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
np.set_printoptions(suppress=True, threshold = np.inf)
import multiprocessing as mp

from sklearn.cross_validation import train_test_split
from sklearn.ensemble import RandomForestClassifier
from sklearn.linear_model import LinearRegression

filenames = []
original_file = {}
cache_size_list = {}
chopt_result = {}
frequency = {}
rd = {}
age = {}

lr_res = {}

def handle_load(file, source_file, catalog, granularity):
	_file = []
	_f = open("../data/sampled/"+catalog+source_file)
	lines = _f.readlines()
	for line in lines:
		_l = line.split("\n")[0].split(",")
		if _l[0] == "page":
			continue
		_file.append(_l[0])
	_f.close()


	chopt_result = {}
	f = open("cache_behavior/" + catalog + granularity + "offline/" + file)
	cache_sizes = []
	while 1:
		line = f.readline()
		line = np.array(line.split("\n")[0].split(" ")[0:-1]).astype(int)
		cache_size = line[0]
		if len(cache_sizes) and cache_size < cache_sizes[-1]:
			break
		else:
			cache_sizes.append(cache_size)
			chopt_result[cache_size] = line[1:]
	f.close()
	# line = f.readline()
	# cache_sizes = np.array(line.split("\n")[0].split(" ")[0:-1]).astype(int)
	# for cache_size in cache_sizes:
	# 	line = f.readline()
	# 	chopt_result[cache_size] = np.array(line.split("\n")[0].split(" ")[0:-1]).astype(int)
	# f.close()

	_frequency = np.zeros((len(chopt_result[cache_sizes[0]]),10))
	for w in range(10000,999,-1000):
		freq = np.loadtxt("../data/frequency/"+catalog+file + "/" + str(w)).astype(int)
		_frequency[:,int((10000-w)/1000)] = freq

	
	_rd = np.loadtxt("../data/rd/"+catalog+file).astype(int)
	_age = np.loadtxt("../data/age/"+catalog+file).astype(int)

	return cache_sizes, chopt_result, _frequency, _file, _rd, _age

# mark: main bottleneck is here at load
def load(catalog, granularity):
	files = os.listdir("../data/sampled/" + catalog)

	temp = []
	for item in files:
		if "canneal" not in item:
			temp.append(item)
	files = temp

	# load chopt result
	pool = mp.Pool(processes=len(files))
	res = {}

	for i in range(len(files)):
		file = files[i].split(".")[0]
		filenames.append(file)
		res[i] = pool.apply_async(handle_load, args=(file, files[i], catalog, granularity))

	pool.close()
	pool.join()

	for i in range(len(filenames)):
		file = filenames[i]		
		_res = res[i].get()
		cache_size_list[file] = _res[0]
		chopt_result[file] = _res[1]
		frequency[file] = _res[2]
		original_file[file] = _res[3]
		rd[file] = _res[4]
		age[file] = _res[5]

def spatial_sample(file, data, res, rate):
	_f = original_file[file]
	_data = np.zeros((data.shape[0], data.shape[1]))
	_res = np.zeros(res.shape)
	count = 0
	for i in range(len(_f)):
		item = _f[i]
		if hash(item) % 100 < 100*rate:
			_data[count,:] = data[i,:]
			_res[count] = res[i]
			count+=1
	return _data[:count,:], _res[:count]


def temporal_sample(data, res, rate):
	count = int(res.shape[0] * rate)
	return data[:count,:], res[:count]


def print_res(catalog, granularity, file, cache_size, res, xtype, option):
	lr_dir = "linear_regression/"
	if not os.path.exists(lr_dir+catalog+granularity+xtype+file+"/"+option):
		os.makedirs(lr_dir+catalog+granularity+xtype+file+"/"+option)

	f = open(lr_dir+catalog+granularity+xtype+file+"/"+option+str(cache_size), "w")
	f.write(np.array2string(res))
	f.close()

def handle_pattern(file, catalog, granularity, xtype):
	for cache_size in cache_size_list[file]:
		res = np.asarray(chopt_result[file][cache_size])
		data = []
		if xtype == "freq/":
			data = np.asarray(frequency[file])
		elif xtype == "all/":
			data = np.zeros((len(frequency[file]), 12))
			data[:,0:10] = np.asarray(frequency[file])
			data[:,10] = np.asarray(age[file])
			data[:,11] = np.asarray(rd[file])

		
		X_train, Y_train= spatial_sample(file, data, res, 0.2)
		lr = LinearRegression()
		lr.fit(X_train, Y_train)
		_res = np.dot(data, np.transpose(np.array(lr.coef_))) + lr.intercept_
		_res = np.where(_res>0.5, 1, 0)
		#print(file, cache_size, 1-(np.count_nonzero(_res-res)) / len(res))
		#print(lr.coef_, lr.intercept_)
		acc = np.count_nonzero(_res-res) / len(res)
		print_res(catalog, granularity, file, cache_size, _res, xtype, "spatial/")

		t_X_train, t_Y_train= temporal_sample(data, res, 0.2)
		t_lr = LinearRegression()
		t_lr.fit(t_X_train, t_Y_train)
		t_res = np.dot(data, np.transpose(np.array(t_lr.coef_))) + t_lr.intercept_
		t_res = np.where(t_res>0.5, 1, 0)
		#print(file, cache_size, 1-(np.count_nonzero(t_res-res)) / len(res))
		#print(t_lr.coef_, t_lr.intercept_)
		t_acc = np.count_nonzero(t_res-res) / len(res)
		print_res(catalog, granularity, file, cache_size, t_res, xtype, "temporal/")
	
	return lr.coef_, lr.intercept_, acc, t_lr.coef_, t_lr.intercept_, t_acc


def pattern(catalog, granularity):
	# given frequency for previous 10 windows (1000 each), see if there's any pattern on chopt result
	
	pool = mp.Pool(processes=len(filenames))
	res = {}

	xtypes = ["freq/", "all/"]

	for i in range(len(filenames)*2):
		file = filenames[int(i/2)]
		xtype = xtypes[int(i%2)]
		res[i] = pool.apply_async(handle_pattern, args=(file, catalog, granularity, xtype))

	pool.close()
	pool.join()

	for i in range(len(filenames)*2):
		file = filenames[int(i/2)]
		xtype = xtypes[int(i%2)]
		_res = res[i].get()
		print(file, xtype, _res[2], _res[5])

	# for file in filenames:
	# 	lr_res[file] = {}
	# 	for cache_size in cache_size_list[file]:
	# 		res = np.asarray(chopt_result[file][cache_size])
	# 		freq = np.asarray(frequency[file])
			

	# 		X_train, Y_train= spatial_sample(file, freq, res, 0.2)
	# 		lr = LinearRegression()
	# 		lr.fit(X_train, Y_train)
	# 		_res = np.dot(freq, np.transpose(np.array(lr.coef_))) + lr.intercept_
	# 		_res = np.where(_res>0.5, 1, 0)
	# 		print(file, cache_size, 1-(np.count_nonzero(_res-res)) / len(res))
	# 		#print(lr.coef_, lr.intercept_)
	# 		print_res(catalog, granularity, file, cache_size, _res, "spatial/")

	# 		t_X_train, t_Y_train= temporal_sample(freq, res, 0.2)
	# 		t_lr = LinearRegression()
	# 		t_lr.fit(t_X_train, t_Y_train)
	# 		t_res = np.dot(freq, np.transpose(np.array(t_lr.coef_))) + t_lr.intercept_
	# 		t_res = np.where(t_res>0.5, 1, 0)
	# 		print(file, cache_size, 1-(np.count_nonzero(t_res-res)) / len(res))
	# 		#print(t_lr.coef_, t_lr.intercept_)
	# 		print_res(catalog, granularity, file, cache_size, t_res, "temporal/")


# def print_lr_res(catalog, granularity):
# 	lr_dir = "linear_regression/"
# 	for file in filenames:
# 		if not os.path.exists(lr_dir+catalog+granularity+file):
# 			os.makedirs(lr_dir+catalog+granularity+file)
# 		for cache_size in cache_size_list[file]:
# 			f = open(lr_dir+catalog+granularity+file+"/"+str(cache_size), "w")
# 			f.write(lr_res[file][cache_size])
# 			f.close()
			


if __name__ == '__main__':
	catalog = "short/"
	granularity = "max/"
	load(catalog, granularity)
	print("load done")
	pattern(catalog, granularity)
	# print_lr_res(catalog, granularity)