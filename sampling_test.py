import os
import sys
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
np.set_printoptions(suppress=True)
from scipy.optimize import curve_fit

import multiprocessing as mp
import operator


cache_size_list = {}
max_cache_size_list = {}
chopt_cost_list = {}
# chopt_cost_list[fbase][sample_rate][sample_order] = chopt_cost

rate_dic = {1:0, 5:1, 10:2, 20:3, 100:4}

sample_rates = {}

def per(x):
	if x < 0:
		x = -x
	return str(int(10000*x)/100) + "\\% & "

def short(x):
	if x < 0:
		x = -x
	return str(int(100*x)/100)

def print_latency_rate(rate_for_error):
	#namelists = ["trace_barnes", "trace_blackscholes", "trace_fluidanimate", "trace_freqmine", "trace_radiosity", "trace_streamcluster"]

	namelists = ["trace_barnes","trace_radiosity","w06","w13","lax_1448_2"]
	if rate_for_error != 0:
		namelists = []
		for temp in chopt_cost_list:
			namelists.append(temp)

	plt.figure(figsize=(30,25))
	count = 1
	err_total = 0
	err_num = 0
	err_max = 0
	for fbase in namelists:
		cache_size = max_cache_size_list[fbase]
		
		order = 1
		sample_rate_list = [1, 5, 10, 20]
		for sample_rate in sample_rate_list:

			if sample_rate == 100:
				continue
			plt.subplot(len(namelists),4,count)
			#plt.subplot(2,2,order)
			i = 1
			ymax = 0
			ymin = 10
			# get highest as for lim
			
			for sample_order in chopt_cost_list[fbase][sample_rate]:
				for item in chopt_cost_list[fbase][sample_rate][sample_order]:
					if item > ymax:
						ymax = item
					if item < ymin and item > 0:
						ymin = item

			for sample_order in chopt_cost_list[fbase][100]:
				for item in chopt_cost_list[fbase][100][sample_rate]:
					if item > ymax:
						ymax = item
					if item < ymin and item > 0:
						ymin = item

			#print(ymin, ymax)
			ymax = (float(int(ymax * 1000)) + 1) / 1000
			ymin = (float(int(ymin * 1000)) - 1) / 1000
			#print(ymin, ymax)

			xs = np.arange(cache_size_list[fbase][sample_rate].shape[0])
			data = np.zeros((len(max_cache_size_list[fbase]), 30))
			for sample_order in range(30):
				if sample_order not in chopt_cost_list[fbase][sample_rate]:
					continue				
				data[xs, sample_order] = chopt_cost_list[fbase][sample_rate][sample_order][0:xs.shape[0]]
					#data[xs+rate_dic[sample_rate],sample_order] = chopt_cost_list[fbase][sample_rate][sample_order]
			data = np.transpose(data)
			if rate_for_error != 0:
				for i in range(chopt_cost_list[fbase][100][sample_rate].shape[0]):
					if sample_rate != rate_for_error:
						continue
					acc = chopt_cost_list[fbase][100][sample_rate][i]
					_err = np.abs(np.max(data[:,i])/acc - 1)
					if _err > err_max and _err <= 0.3:
						err_max = _err
					if _err < 0.3:
						err_total += _err
						err_num += 1

					_err = np.abs(1 - np.min(data[:,i])/acc)
					if _err > err_max and _err <= 0.3:
						err_max = _err
					if _err < 0.3:
						err_total += _err
						err_num += 1

			plt.boxplot(data, notch=False, sym='', vert=True, whis = 1)
			plt.ylim(ymin, ymax)

			if 100 in chopt_cost_list[fbase]:
				#plt.plot(xs, chopt_cost_list[fbase][100][0][0:len(xs)], color='blue', linestyle="-.")
				#plt.plot(xs - np.log(100/sample_rate), chopt_cost_list[fbase][100][sample_rate][0:len(xs)], color='black', linestyle="-.")
				plt.plot(xs+1, chopt_cost_list[fbase][100][sample_rate][0:len(xs)], color='black', linestyle="-.")

			xticks = [ 64 * cache_size_list[fbase][sample_rate][0] * pow(2,i) // 1024 for i in range( 2, xs.shape[0]+2 )] 
			unit = "MB"
			
			plt.xlim(0, len(xticks) + 1)
			plt.xticks(xs+1, xticks)
			plt.xlabel("cache_size (" + unit + ")")
			plt.ylabel("average latency")
			if "trace" in fbase:
				plt.title(fbase+str(sample_rate))
			elif "lax" in fbase:
				plt.title("CDN1" + " " +str(sample_rate))
			else:
				plt.title("Storage"+str(1 if count < 12 else 3) + " " +str(sample_rate))
			count += 1
			order += 1
			# if order == 5:
			# 	break

			f = open("gnuplot_result/sampling_"+fbase+"_"+str(sample_rate)+".dat", "w")
			#f.write("size\tvalue\tavg\tmin\tmax\n")
			for j in range(len(xticks)):
				_size = xticks[j]
				_unit = "MB"
				# if _size >= 1000:
				# 	_size //= 1024
				# 	_unit = "GB"

				f.write(str(_size)+_unit+"\t")
				_latency = int(chopt_cost_list[fbase][100][sample_rate][j]*100000) / 100000
				#print(sample_rate, _size, _unit, _latency)
				f.write(str(_latency)+"\t")
				_data = data[:,j]
				_latency = int(np.mean(_data)*100000) / 100000
				f.write(str(_latency)+"\t")
				_latency = int(np.min(_data)*100000) / 100000
				f.write(str(_latency)+"\t")
				_latency = int(np.max(_data)*100000) / 100000
				f.write(str(_latency)+"\n")

				# _latency = int((chopt/length)[j]*1000) / 1000
				# f.write(str(_latency)+"\t")
				# _latency = int((belady/length)[j]*1000) / 1000
				# f.write(str(_latency)+"\t")
				# _latency = int((lru/length)[j]*1000) / 1000
				# f.write(str(_latency)+"\t")
				# _latency = int((tinylfu/length)[j]*1000) / 1000
				# f.write(str(_latency)+"\t")
				# _latency = int((wtinylfu/length)[j]*1000) / 1000
				# f.write(str(_latency)+"\n")

			f.close()


	# if rate_for_error != 0:
	# 	print(rate_for_error, int(err_max*10000)/10000, err_total, err_num, int(err_total/err_num *10000)/10000)
	# 	if not os.path.exists("sampling_result/"+catalog):
	# 		os.makedirs("sampling_result/"+catalog)
	# 	#plt.savefig("sampling_result/" + catalog + "sampling.png", dpi=300, bbox_inches='tight', pad_inches = 0.0)
	# else:
	# 	plt.savefig("sampling_result/sampling.png", dpi=300, bbox_inches='tight', pad_inches = 0.0)
	# 	plt.close()



def load_simulation_file(root_dir, catalog, granularity):
	_dir = root_dir + catalog + granularity + "offline/"
	temp = os.listdir(_dir)
	#temp = ["trace_blackscholes", "trace_bodytrack", "trace_lu_ncb", "trace_radiosity", "trace_x264", "trace_barnes"]
	#temp = ["trace_fft", "trace_radix"]

	fbase = granularity[0:-1]
	if fbase not in chopt_cost_list:
		chopt_cost_list[fbase] = {}
	if fbase not in cache_size_list:
		cache_size_list[fbase] = {}

	cache_size = []

	for fname in temp:
		# fname: base - sampling rate - order
		name_temp = fname.split("-")
		if len(name_temp) == 1:
			continue

		#sample rate: 1, 5, 10, 20, 100
		sample_rate = int(name_temp[1]) if len(name_temp) > 2 else 100
		if sample_rate not in chopt_cost_list[fbase]:
			chopt_cost_list[fbase][sample_rate] = {}

		#sample order: 0 - 29
		sample_order = int(name_temp[2]) if len(name_temp) > 2 else 0

		file = open(_dir+fname)

		line = file.readline().split("\n")[0].split(" ")
		length = int(line[3])
		# if fbase not in length_list:
		# 	length_list[fbase] = {}
		# if sample_rate not in length_list[fbase][sample_rate]:
		# 	length_list[fbase][sample_rate]
		# length_list[fbase][sample_rate] = length
		# unique_num = int(line[6])
		# unique_num_list[fname] = unique_num

		line = file.readline().split("\n")[0].split(" ")
		# windows = np.array(line[1:-1]).astype(int)
		# windows_list[fname] = windows

		line = file.readline().split("\n")[0].split(" ")
		cache_sizes = np.array(line[2:-1]).astype(int)
		if len(cache_sizes) > len(cache_size):
			cache_size = cache_sizes
			max_cache_size_list[fbase] = cache_size

		if sample_rate not in cache_size_list[fbase] or len(cache_size_list[fbase][sample_rate]) and len(cache_sizes) < len(cache_size_list[fbase][sample_rate]):
			cache_size_list[fbase][sample_rate] = cache_sizes


		line = file.readline().split("\n")[0].split(" ")
		chopt_cost = np.array(line[2:-1]).astype(int)
		chopt_cost_list[fbase][sample_rate][sample_order] = chopt_cost / length

	# if 100 in chopt_cost_list[fbase]:
	# 	temp = {}
	# 	for i in range(len(cache_size_list[fbase][100])):
	# 		temp[cache_size_list[fbase][100][i]] = chopt_cost_list[fbase][100][0][i]
	# 	rate_list = [1,5,10,20]
	# 	for rate in rate_list:
	# 		chopt_cost_list[fbase][100][rate] = []
	# 		for _s in cache_size_list[fbase][rate]:
	# 			index = int(_s * 100 // rate)
	# 			if index in temp:
	# 				chopt_cost_list[fbase][100][rate].append(temp[index])

	# for each sample_rate, cut tail for the same length
	for sample_rate in chopt_cost_list[fbase]:
		for sample_order in chopt_cost_list[fbase][sample_rate]:
			if len(chopt_cost_list[fbase][sample_rate][sample_order] > len(cache_size_list[fbase][sample_rate])):
				chopt_cost_list[fbase][sample_rate][sample_order] = chopt_cost_list[fbase][sample_rate][sample_order][0:len(cache_size_list[fbase][sample_rate])]


	# for all sample rate, append tails to the same max length
	for sample_rate in chopt_cost_list[fbase]:
		for sample_order in chopt_cost_list[fbase][sample_rate]:
			if len(chopt_cost_list[fbase][sample_rate][sample_order]) < len(cache_size):
				chopt_cost_list[fbase][sample_rate][sample_order] = np.append(chopt_cost_list[fbase][sample_rate][sample_order], chopt_cost_list[fbase][sample_rate][sample_order][-1] * np.ones(len(cache_size) - len(chopt_cost_list[fbase][sample_rate][sample_order])))
				#chopt_cost_list[fbase][sample_rate][sample_order] = np.append(chopt_cost_list[fbase][sample_rate][sample_order], np.zeros(len(cache_size) - len(chopt_cost_list[fbase][sample_rate][sample_order])))


	# for sample_rate in chopt_cost_list[fbase]:
	# 	for sample_order in chopt_cost_list[fbase][sample_rate]:
	# 		print(sample_rate, sample_order, len(cache_size_list[fbase][sample_rate]), len(chopt_cost_list[fbase][sample_rate][sample_order]))

def load_root_file(root_dir, catalog, granularity):
	_dir = root_dir + catalog + granularity + "offline/"
	temp = os.listdir(_dir)
	#temp = ["trace_blackscholes", "trace_bodytrack", "trace_lu_ncb", "trace_radiosity", "trace_x264", "trace_barnes"]
	#temp = ["trace_fft", "trace_radix"]

	fbase = granularity[0:-1]
	sample_rate = 100
	cache_size_list[fbase][sample_rate] = {}
	chopt_cost_list[fbase][sample_rate] = {}

	for fname in temp:
		# fname: base - sampling rate - order
		if "-" in fname:
			continue
		name_temp = fname.split("_")

		sample_order = 100 // int(name_temp[-1])

		file = open(_dir+fname)

		line = file.readline().split("\n")[0].split(" ")
		length = int(line[3])
		# if fbase not in length_list:
		# 	length_list[fbase] = {}
		# if sample_rate not in length_list[fbase][sample_rate]:
		# 	length_list[fbase][sample_rate]
		# length_list[fbase][sample_rate] = length
		# unique_num = int(line[6])
		# unique_num_list[fname] = unique_num

		line = file.readline().split("\n")[0].split(" ")
		# windows = np.array(line[1:-1]).astype(int)
		# windows_list[fname] = windows

		line = file.readline().split("\n")[0].split(" ")
		cache_sizes = np.array(line[2:-1]).astype(int)

		cache_size_list[fbase][sample_rate][sample_order] = cache_sizes


		line = file.readline().split("\n")[0].split(" ")
		chopt_cost = np.array(line[2:-1]).astype(int)
		chopt_cost_list[fbase][sample_rate][sample_order] = chopt_cost / length

	for sample_rate in chopt_cost_list[fbase]:
		if sample_rate == 100:
			continue

		for sample_order in chopt_cost_list[fbase][sample_rate]:
			if len(chopt_cost_list[fbase][sample_rate][sample_order]) > len(cache_size_list[fbase][100][sample_rate]):
				chopt_cost_list[fbase][sample_rate][sample_order] = chopt_cost_list[fbase][sample_rate][sample_order][0:len(cache_size_list[fbase][100][sample_rate])]

	max_cache_size_list[fbase] = max_cache_size_list[fbase][1:]
	for sample_rate in chopt_cost_list[fbase]:
		if sample_rate != 100:
			cache_size_list[fbase][sample_rate] = cache_size_list[fbase][sample_rate][1:]
		for sample_order in chopt_cost_list[fbase][sample_rate]:
			#print(sample_rate, sample_order, len(cache_size_list[fbase][sample_rate]), len(chopt_cost_list[fbase][sample_rate][sample_order]))
			chopt_cost_list[fbase][sample_rate][sample_order] = chopt_cost_list[fbase][sample_rate][sample_order][1:]

def print_latency_rate(rate_for_error):
	#namelists = ["trace_barnes", "trace_blackscholes", "trace_fluidanimate", "trace_freqmine", "trace_radiosity", "trace_streamcluster"]

	namelists = ["trace_barnes","trace_radiosity","w06","w13","lax_1448_2"]
	if rate_for_error != 0:
		namelists = []
		for temp in chopt_cost_list:
			namelists.append(temp)

	plt.figure(figsize=(30,25))
	count = 1
	err_total = 0
	err_num = 0
	err_max = 0
	for fbase in namelists:
		cache_size = max_cache_size_list[fbase]
		
		order = 1
		sample_rate_list = [1, 5, 10, 20]
		for sample_rate in sample_rate_list:

			if sample_rate == 100:
				continue
			plt.subplot(len(namelists),4,count)
			#plt.subplot(2,2,order)
			i = 1
			ymax = 0
			ymin = 10
			# get highest as for lim
			
			for sample_order in chopt_cost_list[fbase][sample_rate]:
				for item in chopt_cost_list[fbase][sample_rate][sample_order]:
					if item > ymax:
						ymax = item
					if item < ymin and item > 0:
						ymin = item

			for sample_order in chopt_cost_list[fbase][100]:
				for item in chopt_cost_list[fbase][100][sample_rate]:
					if item > ymax:
						ymax = item
					if item < ymin and item > 0:
						ymin = item

			#print(ymin, ymax)
			ymax = (float(int(ymax * 1000)) + 1) / 1000
			ymin = (float(int(ymin * 1000)) - 1) / 1000
			#print(ymin, ymax)

			xs = np.arange(cache_size_list[fbase][sample_rate].shape[0])
			data = np.zeros((len(max_cache_size_list[fbase]), 30))
			for sample_order in range(30):
				if sample_order not in chopt_cost_list[fbase][sample_rate]:
					continue				
				data[xs, sample_order] = chopt_cost_list[fbase][sample_rate][sample_order][0:xs.shape[0]]
					#data[xs+rate_dic[sample_rate],sample_order] = chopt_cost_list[fbase][sample_rate][sample_order]
			data = np.transpose(data)
			if rate_for_error != 0:
				for i in range(chopt_cost_list[fbase][100][sample_rate].shape[0]):
					if sample_rate != rate_for_error:
						continue
					acc = chopt_cost_list[fbase][100][sample_rate][i]
					_err = np.abs(np.max(data[:,i])/acc - 1)
					if _err > err_max and _err <= 0.3:
						err_max = _err
					if _err < 0.3:
						err_total += _err
						err_num += 1

					_err = np.abs(1 - np.min(data[:,i])/acc)
					if _err > err_max and _err <= 0.3:
						err_max = _err
					if _err < 0.3:
						err_total += _err
						err_num += 1

			plt.boxplot(data, notch=False, sym='', vert=True, whis = 1)
			plt.ylim(ymin, ymax)

			if 100 in chopt_cost_list[fbase]:
				#plt.plot(xs, chopt_cost_list[fbase][100][0][0:len(xs)], color='blue', linestyle="-.")
				#plt.plot(xs - np.log(100/sample_rate), chopt_cost_list[fbase][100][sample_rate][0:len(xs)], color='black', linestyle="-.")
				plt.plot(xs+1, chopt_cost_list[fbase][100][sample_rate][0:len(xs)], color='black', linestyle="-.")

			xticks = [ 64 * cache_size_list[fbase][sample_rate][0] * pow(2,i) // 1024 for i in range( 2, xs.shape[0]+2 )] 
			unit = "MB"
			
			plt.xlim(0, len(xticks) + 1)
			plt.xticks(xs+1, xticks)
			plt.xlabel("cache_size (" + unit + ")")
			plt.ylabel("average latency")
			if "trace" in fbase:
				plt.title(fbase+str(sample_rate))
			elif "lax" in fbase:
				plt.title("CDN1" + " " +str(sample_rate))
			else:
				plt.title("Storage"+str(1 if count < 12 else 3) + " " +str(sample_rate))
			count += 1
			order += 1
			# if order == 5:
			# 	break

			f = open("gnuplot_result/sampling_"+fbase+"_"+str(sample_rate)+".dat", "w")
			#f.write("size\tvalue\tavg\tmin\tmax\n")
			for j in range(len(xticks)):
				_size = xticks[j]
				_unit = "MB"
				# if _size >= 1000:
				# 	_size //= 1024
				# 	_unit = "GB"

				f.write(str(_size)+_unit+"\t")
				_latency = int(chopt_cost_list[fbase][100][sample_rate][j]*100000) / 100000
				#print(sample_rate, _size, _unit, _latency)
				f.write(str(_latency)+"\t")
				_data = data[:,j]
				_latency = int(np.mean(_data)*100000) / 100000
				f.write(str(_latency)+"\t")
				_latency = int(np.min(_data)*100000) / 100000
				f.write(str(_latency)+"\t")
				_latency = int(np.max(_data)*100000) / 100000
				f.write(str(_latency)+"\n")

				# _latency = int((chopt/length)[j]*1000) / 1000
				# f.write(str(_latency)+"\t")
				# _latency = int((belady/length)[j]*1000) / 1000
				# f.write(str(_latency)+"\t")
				# _latency = int((lru/length)[j]*1000) / 1000
				# f.write(str(_latency)+"\t")
				# _latency = int((tinylfu/length)[j]*1000) / 1000
				# f.write(str(_latency)+"\t")
				# _latency = int((wtinylfu/length)[j]*1000) / 1000
				# f.write(str(_latency)+"\n")

			f.close()


	# if rate_for_error != 0:
	# 	print(rate_for_error, int(err_max*10000)/10000, err_total, err_num, int(err_total/err_num *10000)/10000)
	# 	if not os.path.exists("sampling_result/"+catalog):
	# 		os.makedirs("sampling_result/"+catalog)
	# 	#plt.savefig("sampling_result/" + catalog + "sampling.png", dpi=300, bbox_inches='tight', pad_inches = 0.0)
	# else:
	# 	plt.savefig("sampling_result/sampling.png", dpi=300, bbox_inches='tight', pad_inches = 0.0)
	# 	plt.close()




root_dir = "simulation/"

def sampling_result():
	catalog = "sampling_test_new/"
	sample_files = ["trace_barnes/","trace_radiosity/"] 
	#sample_files = os.listdir(root_dir+catalog)
	for sample_file in sample_files:
		load_simulation_file(root_dir, catalog, sample_file)
		load_root_file(root_dir, catalog, sample_file)

	catalog = "sampling_test_cp/"
	sample_files = ["w06/","w13/"]
	#sample_files = os.listdir(root_dir+catalog)
	for sample_file in sample_files:
		load_simulation_file(root_dir, catalog, sample_file)
		load_root_file(root_dir, catalog, sample_file)

	catalog = "sampling_test_akamai/"
	sample_files = ["lax_1448_2/"]
	#sample_files = os.listdir(root_dir+catalog)
	for sample_file in sample_files:
		load_simulation_file(root_dir, catalog, sample_file)
		load_root_file(root_dir, catalog, sample_file)

	print_latency_rate(0)
	#print_latency_rate_statistics(1)

def print_latency_rate_all(rate_for_error, sample_files, catalog):
	namelists=sample_files
	print(namelists)
	count = 1

	f = open("gnuplot_result/"+catalog.split("/")[0]+".dat", "w")
	#stat.write(catalog+"\n")
	latency_all_temp = []
	latency_all_max = []
	latency_all_min = []
	latency_all_avg = []
	sample_rate_list = [1, 5, 10, 20]
	for fbase in namelists:
		if fbase == "trace_graph500_s25_e25" or fbase == "trace_fft" or fbase == "trace_radix":
			continue
		#f.write(fbase+"\n")
		cache_size = max_cache_size_list[fbase]
		
		order = 1
		
		for sample_rate in sample_rate_list:

			if sample_rate == 100:
				continue

			xs = np.arange(cache_size_list[fbase][sample_rate].shape[0])
			data = np.zeros((len(max_cache_size_list[fbase]), 30))
			for sample_order in range(30):
				if sample_order not in chopt_cost_list[fbase][sample_rate]:
					continue				
				data[xs, sample_order] = chopt_cost_list[fbase][sample_rate][sample_order][0:xs.shape[0]]
			data = np.transpose(data)

			xticks = [ 64 * cache_size_list[fbase][sample_rate][0] * pow(2,i) // 1024 for i in range( 2, xs.shape[0]+2 )] 

			latency_temp = []
			latency_max_temp = []
			latency_min_temp = []
			latency_avg_temp = []
			for j in range(len(xticks)):
				_size = xticks[j]
				_latency = int(chopt_cost_list[fbase][100][sample_rate][j]*100000) / 100000
				#f.write(str(_latency)+"\t")
				_data = data[:,j]
				avg_latency = int(np.mean(_data)*100000) / 100000
				max_latency = int(np.amax(_data)*100000) / 100000
				min_latency = int(np.amin(_data)*100000) / 100000

				#print(_latency, avg_latency, max_latency, min_latency)
				#f.write(str(_latency)+"\t")

				# avg / accurate
				#rate = int(100000 * _latency / rate) / 100000
				#max_rate = int(100000 * max_latency / rate) / 100000
				#min_rate = int(100000 * min_latency / rate) / 100000
				#f.write(str(rate)+"\n")
				#if rate - 1 > 0.2:
				#	for _d in _data:
				#		f.write(str(int(1000 *_d)/1000)+" ")
				#f.write("\n")
				
				latency_temp.append(_latency)
				latency_max_temp.append(max_latency if max_latency - _latency < 0.2 else _latency + 0.2)
				latency_min_temp.append(min_latency if _latency - min_latency < 0.2 else _latency - 0.2)
				latency_avg_temp.append(avg_latency)

			#f.write("overall\t" + str(sample_rate)+"\t")
			#f.write(str(np_mean(np.asarray(rate_temp))) + "\n")

			latency_all_temp.append(np_mean(np.asarray(latency_temp)))
			latency_all_max.append(np_mean(np.asarray(latency_max_temp)))
			latency_all_min.append(np_mean(np.asarray(latency_min_temp)))
			latency_all_avg.append(np_mean(np.asarray(latency_avg_temp)))
			
			

	# rate_all_temp: files * 4 sampling rates
	latency_all_temp = np.asarray(latency_all_temp).reshape(len(latency_all_temp)//4,4)
	latency_all_max = np.asarray(latency_all_max).reshape(len(latency_all_max)//4,4)
	latency_all_min = np.asarray(latency_all_min).reshape(len(latency_all_min)//4,4)
	latency_all_avg = np.asarray(latency_all_avg).reshape(len(latency_all_avg)//4,4)

	latency_all_temp = np.mean(latency_all_temp, axis=0)
	latency_all_max = np.mean(latency_all_max, axis=0)
	latency_all_min = np.mean(latency_all_min, axis=0)
	latency_all_avg = np.mean(latency_all_avg, axis=0)


	#acc, mean, min, max
	for i in range(len(sample_rate_list)):
		rate = sample_rate_list[i]
		#print(rate/100, latency_all_temp[i], latency_all_avg[i],latency_all_min[i], latency_all_max[i])
		f.write(str(rate/100)+"\t"+short(latency_all_temp[i]) + "\t" + short(latency_all_avg[i]) + "\t"+ short(latency_all_min[i]) + "\t"+ short(latency_all_max[i]) + "\n")

	f.close()


def sampling_result_all():
	catalog = "sampling_test_new/"
	sample_files = os.listdir(root_dir+catalog)
	for sample_file in sample_files:
		load_simulation_file(root_dir, catalog, sample_file+"/")
		load_root_file(root_dir, catalog, sample_file+"/")
	print_latency_rate_all(1, sample_files, catalog)

	catalog = "sampling_test_cp/"
	sample_files = os.listdir(root_dir+catalog)
	for sample_file in sample_files:
		load_simulation_file(root_dir, catalog, sample_file+"/")
		load_root_file(root_dir, catalog, sample_file+"/")
	print_latency_rate_all(1, sample_files, catalog)

	catalog = "sampling_test_akamai/"
	sample_files = os.listdir(root_dir+catalog)
	for sample_file in sample_files:
		load_simulation_file(root_dir, catalog, sample_file+"/")
		load_root_file(root_dir, catalog, sample_file+"/")
	print_latency_rate_all(1, sample_files, catalog)

def np_mean(x):
	temp = []
	for i in range(x.shape[0]):
		if x[i] != 0:
			temp.append(x[i])
	return np.asarray(temp).mean() if len(temp) else 0


def print_latency_rate_statistics(rate_for_error, sample_files, catalog):
	#namelists = ["trace_barnes","trace_radiosity","w06","w13","lax_1448_2"]
	namelists=sample_files
	print(namelists)

	f = open("sampling_test_accuracy.log", "w")
	stat.write(catalog+"\n")
	rate_all_temp = []
	rate_all_up = []
	rate_all_down = []
	rate_all_gap = []
	for fbase in namelists:
		if fbase == "trace_graph500_s25_e25" or fbase == "trace_fft" or fbase == "trace_radix":
			continue
		f.write(fbase+"\n")
		cache_size = max_cache_size_list[fbase]
		
		order = 1
		sample_rate_list = [1, 5, 10, 20]
		for sample_rate in sample_rate_list:

			if sample_rate == 100:
				continue

			xs = np.arange(cache_size_list[fbase][sample_rate].shape[0])
			data = np.zeros((len(max_cache_size_list[fbase]), 30))
			for sample_order in range(30):
				if sample_order not in chopt_cost_list[fbase][sample_rate]:
					continue				
				data[xs, sample_order] = chopt_cost_list[fbase][sample_rate][sample_order][0:xs.shape[0]]
			data = np.transpose(data)

			xticks = [ 64 * cache_size_list[fbase][sample_rate][0] * pow(2,i) // 1024 for i in range( 2, xs.shape[0]+2 )] 

			rate_temp = []
			rate_max_temp = []
			rate_min_temp = []
			rate_gap_temp = []
			for j in range(len(xticks)):
				_size = xticks[j]
				# accurate result
				_latency = int(chopt_cost_list[fbase][100][sample_rate][j]*100000) / 100000
				rate = _latency
				f.write(str(_latency)+"\t")
				_data = data[:,j]
				# avg result
				_latency = int(np.mean(_data)*100000) / 100000
				max_latency = int(np.amax(_data)*100000) / 100000
				min_latency = int(np.amin(_data)*100000) / 100000
				f.write(str(_latency)+"\t")

				# avg / accurate
				rate = int(100000 * _latency / rate) / 100000
				max_rate = int(100000 * max_latency / rate) / 100000
				min_rate = int(100000 * min_latency / rate) / 100000
				f.write(str(rate)+"\n")
				if rate - 1 > 0.2:
					for _d in _data:
						f.write(str(int(1000 *_d)/1000)+" ")
				f.write("\n")
				
				rate_temp.append(rate)
				rate_gap_temp.append(max_rate - min_rate)

			f.write("overall\t" + str(sample_rate)+"\t")
			f.write(str(np_mean(np.asarray(rate_temp))) + "\n")

			rate_all_temp.append(np_mean(np.asarray(rate_temp)) if np_mean(np.asarray(rate_temp)) < 1.2 else 0)
			rate_all_gap.append(np_mean(np.asarray(rate_gap_temp)) if np_mean(np.asarray(rate_gap_temp)) < 0.2 else 0)
			
	f.close()
	# rate_all_temp: files * 4 sampling rates
	rate_all_temp = np.asarray(rate_all_temp).reshape(len(rate_all_temp)//4,4)
	rate_all_gap = np.asarray(rate_all_gap).reshape(len(rate_all_gap)//4,4)

	temp = []
	for i in range(rate_all_temp.shape[1]):
		temp.append(np.abs(np_mean(rate_all_temp[:,i])-1))
	temp = -np.sort(-np.asarray(temp))
	for i in range(temp.shape[0]):
		stat.write(per(temp[i])+' ')
	stat.write("\n")

	temp = []
	for i in range(rate_all_gap.shape[1]):
		temp.append(np.abs(np_mean(rate_all_gap[:,i])/2))
	temp = -np.sort(-np.asarray(temp))
	for i in range(temp.shape[0]):
		stat.write(per(temp[i])+' ')
	stat.write("\n")

	

def sampling_result_all_size():
	
	catalog = "sampling_test_new/"
	sample_files = os.listdir(root_dir+catalog)
	for sample_file in sample_files:
		load_simulation_file(root_dir, catalog, sample_file+"/")
		load_root_file(root_dir, catalog, sample_file+"/")
	print_latency_rate_statistics(1, sample_files, catalog)

	catalog = "sampling_test_cp/"
	sample_files = os.listdir(root_dir+catalog)
	for sample_file in sample_files:
		load_simulation_file(root_dir, catalog, sample_file+"/")
		load_root_file(root_dir, catalog, sample_file+"/")
	print_latency_rate_statistics(1, sample_files, catalog)

	catalog = "sampling_test_akamai/"
	sample_files = os.listdir(root_dir+catalog)
	for sample_file in sample_files:
		load_simulation_file(root_dir, catalog, sample_file+"/")
		load_root_file(root_dir, catalog, sample_file+"/")
	print_latency_rate_statistics(1, sample_files, catalog)




result_dir = "cache_behavior/"
pattern_dir = "pattern_result/"

filenames = []
offline_cache_types = ["chopt", "belady"]




def handle_sampling_hit_rate(catalog):
	filenames = os.listdir(pattern_dir+catalog)
	print(filenames)

	temp_diff_all = []
	temp_rate_all = []

	for fname in filenames:
		#print(fname)
		if fname == "trace_radix" or fname == "trace_graph500_s25_e25" or fname == "trace_fft":
			continue
		# mark all size and avg hit rate
		# 100: {size:rate}
		# other_rate: {size:[rates]}
		hit_rate_result = {}
		file = open(pattern_dir + catalog + "/" + fname + "/hit_rate.log")
		while(1):
			sample_rate = 0
			sample_order = 0
			line = file.readline()
			if not line:
				break
			name_temp = line.split("\n")[0].split("-")
			if len(name_temp) == 1:
				#root
				sample_rate = 100
				sample_order = 100 // int(name_temp[0].split("_")[-1])
				
				# print(sampling_file, sample_rate)
			else:
				#sample rate: 1, 5, 10, 20, 100
				sample_rate = int(name_temp[1]) if len(name_temp) > 2 else 100

				#sample order: 0 - 29
				sample_order = int(name_temp[2]) if len(name_temp) > 2 else 0
			if sample_rate not in hit_rate_result:
				hit_rate_result[sample_rate]={}
			#print(name_temp)
			#print(sample_rate, sample_order)
			sizes = file.readline().split("\n")[0].split(" ")
			sizes = sizes[4:-1]
			rates = file.readline().split("\n")[0].split(" ")
			rates = rates[4:-1]
			# print(sizes)
			# print(rates)

			for i in range(len(sizes)):
				_size = int(sizes[i])
				_rate = float(rates[i])

				if sample_rate == 100:
					if sample_order not in hit_rate_result[sample_rate]:
						hit_rate_result[sample_rate][sample_order] = {}
					hit_rate_result[sample_rate][sample_order][_size] = _rate
				else:
					if _size not in hit_rate_result[sample_rate]:
						hit_rate_result[sample_rate][_size] = []
					hit_rate_result[sample_rate][_size].append(_rate)

		if 100 not in hit_rate_result:
			continue

		for _rate in hit_rate_result:
			if _rate == 100:
				continue
			for _size in hit_rate_result[_rate]:
				hit_rate_result[_rate][_size] = np.asarray(hit_rate_result[_rate][_size]).mean()
				#print(_rate, _size, hit_rate_result[_rate][_size])

		full = {}
		samp = {}
		
		sample_rate_list = [1, 5, 10, 20]

		for _rate in hit_rate_result[100]:
			if _rate not in full:
				full[_rate] = {}
			for _size in hit_rate_result[100][_rate]:
				full[_rate][_size] = hit_rate_result[100][_rate][_size]


		for _rate in hit_rate_result:
			if _rate == 100:
				continue
			if _rate not in samp:
				samp[_rate] = {}
			for _size in hit_rate_result[_rate]:
				samp[_rate][_size * 100 // _rate] = hit_rate_result[_rate][_size]

		for _rate in sample_rate_list:
			temp = []
			value = []
			if _rate not in samp:
				continue
			for _size in full[_rate]:
				if _size not in samp[_rate]:
					continue
				if full[_rate][_size] - samp[_rate][_size] < 25:
					temp.append(full[_rate][_size] - samp[_rate][_size])
				value.append(full[_rate][_size])
			#print(_rate, np.asarray(temp).mean(), np.asarray(value).mean())
			temp_diff_all.append(np.abs(np_mean(np.asarray(temp))))
			temp_rate_all.append(np.abs(np_mean(np.asarray(value))))

	if not len(temp_diff_all):
		return

	temp_diff_all = np.asarray(temp_diff_all).reshape(len(temp_diff_all)//4, 4)
	temp_rate_all = np.asarray(temp_rate_all).reshape(len(temp_rate_all)//4, 4)

	avg_diff = temp_diff_all.mean(axis=0)
	avg_rate = temp_rate_all.mean(axis=0)

	avg_diff = -np.sort(-np.asarray(avg_diff))
	avg_rate = -np.sort(-np.asarray(avg_rate))

	print(avg_diff)
	print(avg_rate)

	
	hr_stat.write(catalog+"\n")

	for i in range(4):
		hr_stat.write(per(avg_rate[i]/100))
	hr_stat.write("\n")
	for i in range(4):
		hr_stat.write(per(avg_diff[i]/100))
	hr_stat.write("\n")
	


def handle_log_hit_rate(catalog, fname):
	if not os.path.exists(pattern_dir + catalog  + "/" + fname):
 		os.makedirs(pattern_dir + catalog + "/" + fname)
	log = open(pattern_dir + catalog + "/" + fname + "/hit_rate.log", "w")

	sampling_files = os.listdir(result_dir+catalog+"/"+fname+"/offline/")
	
	for sampling_file in sampling_files:			
		try:
			offline_file = np.loadtxt(result_dir+catalog+"/"+fname+"/offline/"+sampling_file).astype(int)
			rates = []
			sizes = []
			for i in range(offline_file.shape[0]):
				cache_type = int(i / (offline_file.shape[0]/len(offline_cache_types)))
				if cache_type == 0:
					cache_size = offline_file[i][0]
					cache_result = offline_file[i][1:]
					# hit rate
					chopt_res = np.where(cache_result % 2 == 1, 1, 0)
					c_rate = np.count_nonzero(chopt_res)/chopt_res.shape[0]
					rates.append(c_rate)
					sizes.append(cache_size)

			if len(rates):
				log.write(sampling_file + "\n    ")
				for item in sizes:
					log.write(str(item) + " ")
				log.write("\n    ")
				for item in rates:
					log.write(str(int(1000*item)/10) + " ")
				log.write("\n")
		except:
			continue
	print(fname, " done")
	log.close()

def log_sampling_hit_rate(catalog):
	filenames = os.listdir(result_dir+catalog)
	print(filenames)

	pool = mp.Pool(processes = len(filenames))
	res = {}
	for i in range(len(filenames)):
		res[i] = pool.apply_async(handle_log_hit_rate, args=(catalog, filenames[i]))

	pool.close()
	pool.join()


def sampling_hit_rate(isLog):	
	if not isLog:
		handle_sampling_hit_rate("sampling_test_new")
		handle_sampling_hit_rate("sampling_test_cp")
		handle_sampling_hit_rate("sampling_test_akamai")
	else:
		if len(sys.argv) > 2:
			log_sampling_hit_rate(sys.argv[2])
		else:
			log_sampling_hit_rate("sampling_test_new")
			log_sampling_hit_rate("sampling_test_cp")
			log_sampling_hit_rate("sampling_test_akamai")

if __name__ == '__main__':
	# catalog = "sampling_test_akamai/"
	# sample_files = os.listdir(root_dir+catalog)	
	# for sample_file in sample_files:
	# 	load_simulation_file(root_dir, catalog, sample_file + "/")
	# 	load_root_file(root_dir, catalog, sample_file + "/")
	# for i in [1,5,10,20]:
	# 	print_latency_rate_new(i)

	workload = sys.argv[1]
	
	if workload == "accuracy":
		sampling_result()

	elif workload == "accuracy_all":
		sampling_result_all()

	elif workload == "stat":
		stat = open("table_result/sampling_acc.table", "w")
		stat.write("1,5,10,20\naverage error bound\nmaximum error bound\n")
		sampling_result_all_size()

	elif workload == "hit_rate_log":
		sampling_hit_rate(1)

	elif workload == "hit_rate":
		hr_stat = open("table_result/sampling_hit_rate.table", "w")
		hr_stat.write("1,5,10,20\naverage hit rate\naverage error bound\n")
		sampling_hit_rate(0)

	#sampling_result()
	#sampling_hit_rate()

	