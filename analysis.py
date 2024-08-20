# change size_offset and imp_size_offset regardly in print_opt_lru_compare_<workload>

import os
import sys
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
np.set_printoptions(suppress=True)
# from sklearn.linear_model import LinearRegression
# from sklearn.metrics import r2_score
# from sklearn import tree
# from sklearn.tree import DecisionTreeClassifier
# from sklearn.ensemble import RandomForestClassifier
# from sklearn.decomposition import PCA
# from sklearn import linear_model
from scipy.optimize import curve_fit

import multiprocessing as mp
import operator

size_offset = 5
# [0.1,	0.2, 0.5,  1,  2,  5,  10, 20, 50, ~100, 100]
# [-10,	-9,  -8,  -7,  -6, -5, -4, -3, -2, -1,   0]
size_list = [0,1,3]

file_list = []
length_list = {}
unique_num_list = {}
windows_list = {}
cache_size_list = {}
chopt_cost_list = {}
chopt_samerw_flag = False
chopt_samerw_cost_list = {}
belady_cost_list = {}
beladyac_cost_list = {}
#static_cost_list = {}
lru_cost_list = {}
lfu_cost_list = {}
tinylfu_cost_list = {}
wtinylfu_cost_list = {}
tinylfu_only_cost_list = {}
slru_cost_list = {}
offline_spatial_cost_list = {}
offline_temporal_cost_list = {}

matrix = {}

columns_list = {}

legend_list=["r/w", "isScan", "rd", "age", "max size/64", "max size/32", "max size/16", "max size/8","max size/4", "max size/2", "max size", "2*max size", "4*max size", "8*max size"]

original_perf = {}
simplified_perf = {}

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
			# if "trace" in _file:
			# 	sample_rates[_file] *= 64
			# else:
			# 	sample_rates[_file] *= 16
			if "trace" in _file:
				sample_rates[_file] *= 4
			else:
				sample_rates[_file] *= 64
	# for fname in sample_rates:
	# 	print(fname, sample_rates[fname])

def fit(self, X, a, b, c, d):
    x,y,z=X
    return a*x+ b*np.log(y) + c*np.log(z) + d

def r2(y1, y2):
	return round(1-np.var(y1-y2)/np.var(y1), 3)


def per(x):
	return str(int(1000*x)/10) + "\\%"

def print_opt_lru_compare_parsec(catalog,granularity,calc_type,imp_size_offset):
	head = "filename\t0.1\\%\t0.5\\%\t1\\%\n"
	if catalog == "parsec2/":
		catalog = "parsec/"

	if not calc_type:
		f_b = open("gnuplot_result/chopt_belady_" + catalog.split("/")[0] + ".dat", "w")
		f_bac = open("gnuplot_result/chopt_beladyac_" + catalog.split("/")[0] + ".dat", "w")
		f_l = open("gnuplot_result/chopt_lru_" + catalog.split("/")[0] + ".dat", "w")
		f_t = open("gnuplot_result/chopt_tinylfu_" + catalog.split("/")[0] + ".dat", "w")
		f_w = open("gnuplot_result/chopt_wtinylfu_" + catalog.split("/")[0] +".dat", "w")
		f_b.write(head)
		f_bac.write(head)
		f_l.write(head)
		f_t.write(head)
		f_w.write(head)

	count = 1
	
	maximum_b = 0
	total_b = 0
	maximum_bac = 0
	total_bac = 0
	maximum_t = 0
	total_t = 0
	maximum_w = 0
	total_w = 0
	maximum_l = 0
	total_l = 0

	for i in range(len(file_list)):
		fname = file_list[i]
		# if fname not in sample_rates:
		# 	continue
		# print(fname)
		length = length_list[fname]
		unique_num = unique_num_list[fname]

		c=cache_size_list[fname] * sample_rates[fname]
		unit = "B"
		if c[4] > 1024:
			c = c // 1024
			unit = "KB"
		if c[4] > 1000:
			c = c // 1000
			unit = "MB"

		chopt=chopt_cost_list[fname]
		chopt_samerw=[]
		if chopt_samerw_flag:
			chopt_samerw=chopt_samerw_cost_list[fname]
		belady=belady_cost_list[fname]
		beladyac=beladyac_cost_list[fname]
		#static=static_cost_list[fname]
		lru=lru_cost_list[fname]
		lfu=lfu_cost_list[fname]
		tinylfu=tinylfu_cost_list[fname]
		wtinylfu=wtinylfu_cost_list[fname]
		tinylfu_only=tinylfu_only_cost_list[fname]
		slru=slru_cost_list[fname]
		

		size_offset = 9 if 9 <= chopt.shape[0] else chopt.shape[0]
		size_list = [0,2,3]
		imp_size_offset = 7 if 7 < chopt.shape[0] else chopt.shape[0]


		if fname == "trace_graph500_s25_e25":
			fname = "trace_graph500"

		if not calc_type:

			f_b.write(fname.replace("_", "-").replace("trace-","") + "\t")
			for j in size_list:
				improvement = 1-(chopt/belady)[chopt.shape[0]+j-size_offset]
				f_b.write( str(int(improvement*100000)/1000))
				f_b.write("\t" if j < size_list[-1] else "\n")

			f_bac.write(fname.replace("_", "-").replace("trace-","") + "\t")
			for j in size_list:
				improvement = 1-(chopt/beladyac)[chopt.shape[0]+j-size_offset]
				f_bac.write( str(int(improvement*100000)/1000))
				f_bac.write("\t" if j < size_list[-1] else "\n")

			f_l.write(fname.replace("_", "-").replace("trace-","") + "\t")
			for j in size_list:
				improvement = 1-(chopt/lru)[chopt.shape[0]+j-size_offset]
				f_l.write( str(int(improvement*100000)/1000))
				f_l.write("\t" if j < size_list[-1] else "\n")
				
			f_t.write(fname.replace("_", "-").replace("trace-","") + "\t")
			for j in size_list:
				improvement = 1-(chopt/tinylfu)[chopt.shape[0]+j-size_offset]
				f_t.write( str(int(improvement*100000)/1000))
				f_t.write("\t" if j < size_list[-1] else "\n")
				
			f_w.write(fname.replace("_", "-").replace("trace-","") + "\t")
			for j in size_list:
				improvement = 1-(chopt/wtinylfu)[chopt.shape[0]+j-size_offset]
				f_w.write( str(int(improvement*100000)/1000))
				f_w.write("\t" if j < size_list[-1] else "\n")

		else:
			for k in range(chopt.shape[0]):
				improvement = 1-(chopt/belady)[k]
				total_b += improvement/chopt.shape[0]
				if improvement > maximum_b:
					maximum_b = improvement

				improvement = 1-(chopt/beladyac)[k]
				total_bac += improvement/chopt.shape[0]
				if improvement > maximum_bac:
					maximum_bac = improvement
				
				improvement = 1-(chopt/tinylfu)[k]
				total_t += improvement/chopt.shape[0]
				if improvement > maximum_t:
					maximum_t = improvement
				
				improvement = 1-(chopt/wtinylfu)[k]
				total_w += improvement/chopt.shape[0]
				if improvement > maximum_w:
					maximum_w = improvement

				improvement = 1-(chopt/lru)[k]
				total_l += improvement/chopt.shape[0]
				if improvement > maximum_l:
					maximum_l = improvement

			# improvement = 1-(chopt/belady)[chopt.shape[0]-imp_size_offset]
			# total_b += improvement
			# if improvement > maximum_b:
			# 	maximum_b = improvement

			# improvement = 1-(chopt/beladyac)[chopt.shape[0]-imp_size_offset]
			# total_bac += improvement
			# if improvement > maximum_bac:
			# 	maximum_bac = improvement
			
			# improvement = 1-(chopt/tinylfu)[chopt.shape[0]-imp_size_offset]
			# total_t += improvement
			# if improvement > maximum_t:
			# 	maximum_t = improvement
			
			# improvement = 1-(chopt/wtinylfu)[chopt.shape[0]-imp_size_offset]
			# total_w += improvement
			# if improvement > maximum_w:
			# 	maximum_w = improvement

			# improvement = 1-(chopt/lru)[chopt.shape[0]-imp_size_offset]
			# total_l += improvement
			# if improvement > maximum_l:
			# 	maximum_l = improvement

	
		count+=1

	if calc_type:
		stat.write(catalog+" "+granularity+"\n")
		stat.write(per(total_b/len(file_list)) + " & ")
		stat.write(per(maximum_b) + " & ")
		stat.write(per(total_bac/len(file_list)) + " & ")
		stat.write(per(maximum_bac) + " & ")
		stat.write(per(total_l/len(file_list)) + " & ")
		stat.write(per(maximum_l) + " & ")
		stat.write(per(total_t/len(file_list)) + " & ")
		stat.write(per(maximum_t) + "\n")
		# stat.write(per(total_w/len(file_list)) + " & ")
		# stat.write(per(maximum_w) + "\n")

	

def print_opt_lru_compare_cp(catalog,granularity, calc_type, imp_size_offset):
	head = "filename\t1\\%\t2\\%\t5\\%\n"
	# size_offset = 7
	# size_list = [0,1,2]
	# imp_size_offset = 7
	if not calc_type:
		f_b = open("gnuplot_result/chopt_belady_" + catalog.split("/")[0] + ".dat", "w")
		f_bac = open("gnuplot_result/chopt_beladyac_" + catalog.split("/")[0] + ".dat", "w")
		f_l = open("gnuplot_result/chopt_lru_" + catalog.split("/")[0] + ".dat", "w")
		f_t = open("gnuplot_result/chopt_tinylfu_" + catalog.split("/")[0] + ".dat", "w")
		f_w = open("gnuplot_result/chopt_wtinylfu_" + catalog.split("/")[0] +".dat", "w")
		f_b.write(head)
		f_bac.write(head)
		f_l.write(head)
		f_t.write(head)
		f_w.write(head)
	
	namelist = ["w01", "w03", "w06", "w09", "w13", "w30"]
	#plt.figure(figsize=(20,10))
	# fig1 = plt.figure(figsize=(20,10))
	# fig2 = plt.figure(figsize=(20,10))
	count = 0

	maximum_b = 0
	total_b = 0
	maximum_bac = 0
	total_bac = 0
	maximum_t = 0
	total_t = 0
	maximum_w = 0
	total_w = 0
	maximum_l = 0
	total_l = 0


	file_list_new = ["w06_vscsi1", "w09_vscsi1", "w13_vscsi1"]
	for _fname in file_list:
		if _fname not in file_list_new:
			file_list_new.append(_fname)


	for i in range(len(file_list_new)):
		fname = file_list_new[i]

		length = length_list[fname]
		unique_num = unique_num_list[fname]

		c=cache_size_list[fname] * sample_rates[fname]
		unit = "KB"
		if c[4] > 1024:
			c = c // 1024
			unit = "MB"
		if c[3] > 1000:
			c = c // 1000
			unit = "GB"

		chopt=chopt_cost_list[fname]
		chopt_samerw=[]
		if chopt_samerw_flag:
			chopt_samerw=chopt_samerw_cost_list[fname]
		belady=belady_cost_list[fname]
		beladyac=beladyac_cost_list[fname]
		#static=static_cost_list[fname]
		lru=lru_cost_list[fname]
		lfu=lfu_cost_list[fname]
		tinylfu=tinylfu_cost_list[fname]
		wtinylfu=wtinylfu_cost_list[fname]
		tinylfu_only=tinylfu_only_cost_list[fname]
		slru=slru_cost_list[fname]

		size_offset = 7 if 7 <= chopt.shape[0] else chopt.shape[0]
		size_list = [0,1,2]
		imp_size_offset = 7 if 7 < chopt.shape[0] else chopt.shape[0]
		
		if not calc_type:
			if i >= 10:
				continue
			f_b.write("Storage"+str(i+1) + "\t")
			for j in size_list:
				improvement = 1-(chopt/belady)[chopt.shape[0]+j-size_offset]
				f_b.write( str(int(improvement*100000)/1000))
				f_b.write("\t" if j < size_list[-1] else "\n")

			f_bac.write("Storage"+str(i+1) + "\t")
			for j in size_list:
				improvement = 1-(chopt/beladyac)[chopt.shape[0]+j-size_offset]
				f_bac.write( str(int(improvement*100000)/1000))
				f_bac.write("\t" if j < size_list[-1] else "\n")

			f_l.write("Storage"+str(i+1) + "\t")
			for j in size_list:
				improvement = 1-(chopt/lru)[chopt.shape[0]+j-size_offset]
				f_l.write( str(int(improvement*100000)/1000))
				f_l.write("\t" if j < size_list[-1] else "\n")
				
			f_t.write("Storage"+str(i+1) + "\t")
			for j in size_list:
				improvement = 1-(chopt/tinylfu)[chopt.shape[0]+j-size_offset]
				f_t.write( str(int(improvement*100000)/1000))
				f_t.write("\t" if j < size_list[-1] else "\n")
				
			f_w.write("Storage"+str(i+1) + "\t")
			for j in size_list:
				improvement = 1-(chopt/wtinylfu)[chopt.shape[0]+j-size_offset]
				f_w.write( str(int(improvement*100000)/1000))
				f_w.write("\t" if j < size_list[-1] else "\n")


		else: 
			for k in range(chopt.shape[0]):
				improvement = 1-(chopt/belady)[k]
				total_b += improvement/chopt.shape[0]
				if improvement > maximum_b:
					maximum_b = improvement

				improvement = 1-(chopt/beladyac)[k]
				total_bac += improvement/chopt.shape[0]
				if improvement > maximum_bac:
					maximum_bac = improvement
				
				improvement = 1-(chopt/tinylfu)[k]
				total_t += improvement/chopt.shape[0]
				if improvement > maximum_t:
					maximum_t = improvement
				
				improvement = 1-(chopt/wtinylfu)[k]
				total_w += improvement/chopt.shape[0]
				if improvement > maximum_w:
					maximum_w = improvement

				improvement = 1-(chopt/lru)[k]
				total_l += improvement/chopt.shape[0]
				if improvement > maximum_l:
					maximum_l = improvement

			# improvement = 1-(chopt/belady)[chopt.shape[0]-imp_size_offset]
			# total_b += improvement
			# if improvement > maximum_b:
			# 	maximum_b = improvement

			# improvement = 1-(chopt/beladyac)[chopt.shape[0]-imp_size_offset]
			# total_bac += improvement
			# if improvement > maximum_bac:
			# 	maximum_bac = improvement
			
			# improvement = 1-(chopt/tinylfu)[chopt.shape[0]-imp_size_offset]
			# total_t += improvement
			# if improvement > maximum_t:
			# 	maximum_t = improvement
			
			# improvement = 1-(chopt/wtinylfu)[chopt.shape[0]-imp_size_offset]
			# total_w += improvement
			# if improvement > maximum_w:
			# 	maximum_w = improvement

			# improvement = 1-(chopt/lru)[chopt.shape[0]-imp_size_offset]
			# total_l += improvement
			# if improvement > maximum_l:
			# 	maximum_l = improvement

		count+=1
		

		# if fname.split("_")[0] not in namelist:
		# 	continue


	if calc_type:
		stat.write(catalog+" "+granularity+"\n")
		stat.write(per(total_b/count) + " & ")
		stat.write(per(maximum_b) + " & ")
		stat.write(per(total_bac/count) + " & ")
		stat.write(per(maximum_bac) + " & ")
		stat.write(per(total_l/count) + " & ")
		stat.write(per(maximum_l) + " & ")
		stat.write(per(total_t/count) + " & ")
		stat.write(per(maximum_t) + "\n")
		# stat.write(per(total_w/count) + " & ")
		# stat.write(per(maximum_w) + "\n")


	

def print_opt_lru_compare_akamai(catalog,granularity, calc_type, imp_size_offset):
	head = "filename\t1\\%\t2\\%\t5\\%\n"
	# size_offset = 7
	# size_list = [0,1,2]
	# imp_size_offset = 7
	#plt.figure(figsize=(30,15))
	# fig1 = plt.figure(figsize=(30,15))
	# fig2 = plt.figure(figsize=(30,15))
	if not calc_type:
		f_b = open("gnuplot_result/chopt_belady_" + catalog.split("/")[0] + ".dat", "w")
		f_bac = open("gnuplot_result/chopt_beladyac_" + catalog.split("/")[0] + ".dat", "w")
		f_l = open("gnuplot_result/chopt_lru_" + catalog.split("/")[0] + ".dat", "w")
		f_t = open("gnuplot_result/chopt_tinylfu_" + catalog.split("/")[0] + ".dat", "w")
		f_w = open("gnuplot_result/chopt_wtinylfu_" + catalog.split("/")[0] +".dat", "w")
		f_b.write(head)
		f_bac.write(head)
		f_l.write(head)
		f_t.write(head)
		f_w.write(head)
	count = 1

	maximum_b = 0
	total_b = 0
	maximum_bac = 0
	total_bac = 0
	maximum_t = 0
	total_t = 0
	maximum_w = 0
	total_w = 0
	maximum_l = 0
	total_l = 0

	file_list_new = ["lax_1448_2", "lax_1448_3", "lax_1448_5"]
	for _fname in file_list:
		if _fname not in file_list_new:
			file_list_new.append(_fname)

	

	for i in range(len(file_list_new)):
		fname = file_list_new[i]
		#print(fname)

		length = length_list[fname]
		unique_num = unique_num_list[fname]

		c=cache_size_list[fname] * sample_rates[fname]
		unit = "MB"
		if c[4] > 1000:
			c = c // 1000
			unit = "GB"
		# if c[4] > 1000:
		# 	c = c // 1000
		# 	unit = "GB"

		chopt=chopt_cost_list[fname]
		chopt_samerw=[]
		if chopt_samerw_flag:
			chopt_samerw=chopt_samerw_cost_list[fname]
		belady=belady_cost_list[fname]
		beladyac=beladyac_cost_list[fname]
		#static=static_cost_list[fname]
		lru=lru_cost_list[fname]
		lfu=lfu_cost_list[fname]
		tinylfu=tinylfu_cost_list[fname]
		wtinylfu=wtinylfu_cost_list[fname]
		tinylfu_only=tinylfu_only_cost_list[fname]
		slru=slru_cost_list[fname]

		size_offset = 7 if 7 <= chopt.shape[0] else chopt.shape[0]
		size_list = [0,1,2]
		imp_size_offset = 7 if 7 < chopt.shape[0] else chopt.shape[0]
		
		if not calc_type:

			f_b.write("CDN"+str(i+1) + "\t")
			for j in size_list:
				improvement = 1-(chopt/belady)[chopt.shape[0]+j-size_offset]
				f_b.write( str(int(improvement*100000)/1000))
				f_b.write("\t" if j < size_list[-1] else "\n")

			f_bac.write("CDN"+str(i+1) + "\t")
			for j in size_list:
				improvement = 1-(chopt/beladyac)[chopt.shape[0]+j-size_offset]
				f_bac.write( str(int(improvement*100000)/1000))
				f_bac.write("\t" if j < size_list[-1] else "\n")

			f_l.write("CDN"+str(i+1)+ "\t")
			for j in size_list:
				improvement = 1-(chopt/lru)[chopt.shape[0]+j-size_offset]
				f_l.write( str(int(improvement*100000)/1000))
				f_l.write("\t" if j < size_list[-1] else "\n")
				
			f_t.write("CDN"+str(i+1) + "\t")
			for j in size_list:
				improvement = 1-(chopt/tinylfu)[chopt.shape[0]+j-size_offset]
				f_t.write( str(int(improvement*100000)/1000))
				f_t.write("\t" if j < size_list[-1] else "\n")
				
			f_w.write("CDN"+str(i+1) + "\t")
			for j in size_list:
				improvement = 1-(chopt/wtinylfu)[chopt.shape[0]+j-size_offset]
				f_w.write( str(int(improvement*100000)/1000))
				f_w.write("\t" if j < size_list[-1] else "\n")

		else:
			for k in range(chopt.shape[0]):
				improvement = 1-(chopt/belady)[k]
				total_b += improvement/chopt.shape[0]
				if improvement > maximum_b:
					maximum_b = improvement

				improvement = 1-(chopt/beladyac)[k]
				total_bac += improvement/chopt.shape[0]
				if improvement > maximum_bac:
					maximum_bac = improvement
				
				improvement = 1-(chopt/tinylfu)[k]
				total_t += improvement/chopt.shape[0]
				if improvement > maximum_t:
					maximum_t = improvement
				
				improvement = 1-(chopt/wtinylfu)[k]
				total_w += improvement/chopt.shape[0]
				if improvement > maximum_w:
					maximum_w = improvement

				improvement = 1-(chopt/lru)[k]
				total_l += improvement/chopt.shape[0]
				if improvement > maximum_l:
					maximum_l = improvement
			


			# improvement = 1-(chopt/belady)[chopt.shape[0]-imp_size_offset]
			# total_b += improvement
			# if improvement > maximum_b:
			# 	maximum_b = improvement

			# improvement = 1-(chopt/beladyac)[chopt.shape[0]-imp_size_offset]
			# total_bac += improvement
			# if improvement > maximum_bac:
			# 	maximum_bac = improvement
			
			# improvement = 1-(chopt/tinylfu)[chopt.shape[0]-imp_size_offset]
			# total_t += improvement
			# if improvement > maximum_t:
			# 	maximum_t = improvement
			
			# improvement = 1-(chopt/wtinylfu)[chopt.shape[0]-imp_size_offset]
			# total_w += improvement
			# if improvement > maximum_w:
			# 	maximum_w = improvement

			# improvement = 1-(chopt/lru)[chopt.shape[0]-imp_size_offset]
			# total_l += improvement
			# if improvement > maximum_l:
			# 	maximum_l = improvement

		count+=1

	if calc_type:
		stat.write(catalog+" "+granularity+"\n")
		stat.write(per(total_b/len(file_list)) + " & ")
		stat.write(per(maximum_b) + " & ")
		stat.write(per(total_bac/len(file_list)) + " & ")
		stat.write(per(maximum_bac) + " & ")
		stat.write(per(total_l/len(file_list)) + " & ")
		stat.write(per(maximum_l) + " & ")
		stat.write(per(total_t/len(file_list)) + " & ")
		stat.write(per(maximum_t) + "\n")
		# stat.write(per(total_w/len(file_list)) + " & ")
		# stat.write(per(maximum_w) + "\n")

chopt_rw = {}
belady_rw = {}
beladyac_rw = {}
lru_rw = {}
tinylfu_rw = {}

def loadrw(catalog,granularity):
	if catalog not in chopt_rw:
		chopt_rw[catalog] = {}
		belady_rw[catalog] = {}
		beladyac_rw[catalog] = {}
		lru_rw[catalog] = {}
		tinylfu_rw[catalog] = {}
	for i in range(len(file_list)):
		fname = file_list[i]
		if fname not in chopt_rw[catalog]:
			chopt_rw[catalog][fname] = []
			belady_rw[catalog][fname] = {}
			beladyac_rw[catalog][fname] = {}
			lru_rw[catalog][fname] = {}
			tinylfu_rw[catalog][fname] = {}
		chopt_rw[catalog][fname].append(chopt_cost_list[fname])
		belady_rw[catalog][fname].append(belady_cost_list[fname])
		beladyac_rw[catalog][fname].append(beladyac_cost_list[fname])
		lru_rw[catalog][fname].append(lru_cost_list[fname])
		tinylfu_rw[catalog][fname].append(tinylfu_cost_list[fname])

def handlerw():
	rw = open("table_result/rw.table", "w")
	rw.write("Asymmetry\nAverage Maximum\n")
	for catalog in chopt_rw:
		print(catalog)
		for fname in chopt_rw[catalog]:
			no_rw = chopt_rw[catalog][fname][0]
			with_rw =  chopt_rw[catalog][fname][1]
			
			_l = len(no_rw) if len(no_rw) < len(with_rw) else len(with_rw)

			improvement = np.asarray(no_rw[:_l]/with_rw[_l] )
			print("  ",fname, np.mean(improvement-1))
			


	

def print_opt_lru_compare_all():
	#plt.figure(figsize=(30,15))
	fig1 = plt.figure(figsize=(30,20))
	fig2 = plt.figure(figsize=(30,20))
	count = 1

	target_list = ["trace_streamcluster", "trace_radiosity", "trace_ocean_ncp", "trace_graph500_s25_e25", "w06_vscsi1", "w09_vscsi1", "w13_vscsi1", "lax_1448_2", "lax_1448_3", "lax_1448_5"]

	for i in range(len(target_list)):
		fname = target_list[i]
		#print(fname)

		length = length_list[fname]
		unique_num = unique_num_list[fname]

		c=cache_size_list[fname] * sample_rates[fname]
		# unit = "KB"
		# if c[4] > 1024:
		# 	c = c // 1024
		# 	unit = "MB"
		# if c[4] > 1000:
		# 	c = c // 1000
		# 	unit = "GB"


		chopt=chopt_cost_list[fname]
		chopt_samerw=[]
		if chopt_samerw_flag:
			chopt_samerw=chopt_samerw_cost_list[fname]
		belady=belady_cost_list[fname]
		beladyac=beladyac_cost_list[fname]
		#static=static_cost_list[fname]
		lru=lru_cost_list[fname]
		lfu=lfu_cost_list[fname]
		tinylfu=tinylfu_cost_list[fname]
		wtinylfu=wtinylfu_cost_list[fname]
		tinylfu_only=tinylfu_only_cost_list[fname]
		slru=slru_cost_list[fname]


		f = open("gnuplot_result/avg_"+fname+".dat", "w")
		f.write("size\tCHOPT\tBelady\tBelady-AD\tLRU\tW-TinyLFU\n")
		for j in range(len(c)):
			if c[j] != c[j-1]*2:
				continue
			_size = c[j]
			_unit = "KB"
			if i < 4:
				_unit = "KB"
				#_size *= 4
				if _size > 1024:
					_size = _size // 1024
					_unit = "MB"
				if _size > 1000:
					_size = _size // 1000
					_unit = "GB"
			elif i > 3 and i < 7:
				_unit = "KB"
				#_size *= 64
				if _size > 1024:
					_size = _size // 1024
					_unit = "MB"
				if _size > 1000:
					_size = _size // 1000
					_unit = "GB"
			elif i > 6:
				_unit = "MB"
				#_size *= 64 
				if _size > 1000:
					_size = _size // 1000
					_unit = "GB"
			# if _size >= 1024:
			# 	_size //= 1024
			# 	_unit = "MB"
			# if _size >= 1000:
			# 	_size //= 1000
			# 	_unit = "GB"
			f.write(str(_size)+_unit+"\t")
			_latency = int((chopt/length)[j]*1000) / 1000
			f.write(str(_latency)+"\t")
			_latency = int((belady/length)[j]*1000) / 1000
			f.write(str(_latency)+"\t")
			_latency = int((beladyac/length)[j]*1000) / 1000
			f.write(str(_latency)+"\t")
			_latency = int((lru/length)[j]*1000) / 1000
			f.write(str(_latency)+"\t")
			if i > -1:
				_latency = int((tinylfu/length)[j]*1000) / 1000
				f.write(str(_latency)+"\n")
			else:
				_latency = int((wtinylfu/length)[j]*1000) / 1000
				f.write(str(_latency)+"\n")

		f.close()


		count+=1


def check_file():
	frequency = {}
	for i in range(matrix["trace_fft"].shape[0]):
		item = matrix["trace_fft"][i][0]
		if item in frequency:
			frequency[item] += 1
		else:
			frequency[item] = 1

	inverse_frequency = {}

	for k in frequency:
		if frequency[k] in inverse_frequency:
			inverse_frequency[frequency[k]] += 1
		else:
			inverse_frequency[frequency[k]] = 1

	print(inverse_frequency)

		
def load_simulation_file(root_dir, catalog, granularity, option):
	_dir = root_dir + catalog + granularity + option
	temp = os.listdir(_dir)
	#temp = ["trace_blackscholes", "trace_bodytrack", "trace_lu_ncb", "trace_radiosity", "trace_x264", "trace_barnes"]
	#temp = ["trace_fft", "trace_radix"]
	_list = []
	for item in temp:
		if option == "online/":
			file_list.append(item)
		_list.append(item)

	offline_spatial_cost_list[0] = {}
	offline_spatial_cost_list[1] = {}
	offline_temporal_cost_list[0] = {}
	offline_temporal_cost_list[1] = {}

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
		#print(fname)

		if option == "online/" or option ==  "offline/":
			line = file.readline().split("\n")[0].split(" ")
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

			#print(fname, "online loaded")

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

			#print(fname, "offline loaded")

		elif option == "custom/":
			line = file.readline().split("\n")[0].split(" ")
			offline_spatial_cost = np.array(line[3:-1]).astype(int)
			offline_spatial_cost_list[0][fname] = offline_spatial_cost

			line = file.readline().split("\n")[0].split(" ")
			offline_temporal_cost = np.array(line[3:-1]).astype(int)
			offline_temporal_cost_list[0][fname] = offline_temporal_cost

			line = file.readline().split("\n")[0].split(" ")
			offline_spatial_cost = np.array(line[3:-1]).astype(int)
			offline_spatial_cost_list[1][fname] = offline_spatial_cost

			line = file.readline().split("\n")[0].split(" ")
			offline_temporal_cost = np.array(line[3:-1]).astype(int)
			offline_temporal_cost_list[1][fname] = offline_temporal_cost

			#print(fname, "custom loaded")

def load_samerw(root_dir, catalog, granularity):
	_dir = root_dir + catalog + granularity + "offline/"

	for fname in file_list:
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
		line = file.readline()
		line = file.readline()
		line = file.readline()

		line = file.readline().split("\n")[0].split(" ")
		chopt_samerw_cost = np.array(line[2:-1]).astype(int)
		print("samerw ", fname)
		chopt_samerw_cost_list[fname] = chopt_samerw_cost



if __name__ == '__main__':
	get_sample_rate()
	root_dir = "simulation/"

	workload = sys.argv[1]

	if workload == "parsec":
		catalog = "parsec2/"
		granularity = "sampled_parsec/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		catalog = "parsec/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		file_list.sort()
		print_opt_lru_compare_parsec(catalog, granularity,0,2)

	elif workload == "cp":
		catalog = "cp/"
		granularity = "cp/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		file_list.sort()
		print_opt_lru_compare_cp(catalog, granularity,0,2)

	elif workload == "akamai":
		catalog = "akamai/"
		granularity = "samerw/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		file_list.sort()
		print_opt_lru_compare_akamai(catalog, granularity,0,2)

	elif workload == "latency":
		catalog = "parsec2/"
		granularity = "sampled_parsec/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		catalog = "parsec/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		catalog = "cp/"
		granularity = "cp/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		catalog = "akamai/"
		granularity = "akamai/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		file_list.sort()
		print_opt_lru_compare_all()

	elif workload == "improvement":
		catalog = "parsec2/"
		granularity = "sampled_parsec/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		catalog = "parsec/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		print_opt_lru_compare_parsec(catalog, granularity, 0, 2)
		file_list = []
		catalog = "cp/"
		granularity = "cp/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		print_opt_lru_compare_cp(catalog, granularity, 0, 2)
		file_list = []
		catalog = "akamai/"
		granularity = "akamai/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		print_opt_lru_compare_akamai(catalog, granularity, 0, 2)

	elif workload == "stat":
		# f_b.write("filename\t25\\%\t50\\%\t75\\%\n")
		# f_l.write("filename\t25\\%\t50\\%\t75\\%\n")
		# f_t.write("filename\t25\\%\t50\\%\t75\\%\n")
		# f_w.write("filename\t25\\%\t50\\%\t75\\%\n")
		stat = open("table_result/improvement.table", "w")
		stat.write("CHOPT over Belady, BeladyAD, LRU, W-TinyLFU\nAverage Maximum\n")
		catalog = "parsec2/"
		granularity = "sampled_parsec/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		catalog = "parsec/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		print_opt_lru_compare_parsec(catalog, granularity, 1, 2)
		file_list = []
		catalog = "parsec2/"
		granularity = "samerw/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		catalog = "parsec/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		print_opt_lru_compare_parsec(catalog, granularity, 1, 2)
		file_list = []

		catalog = "cp/"
		granularity = "cp/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		print_opt_lru_compare_cp(catalog, granularity, 1, 2)
		file_list = []
		catalog = "cp/"
		granularity = "samerw/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		print_opt_lru_compare_cp(catalog, granularity, 1, 2)
		file_list = []

		catalog = "akamai/"
		granularity = "akamai/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		print_opt_lru_compare_akamai(catalog, granularity, 1,2)
		file_list=[]
		catalog = "akamai/"
		granularity = "samerw/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		print_opt_lru_compare_akamai(catalog, granularity, 1, 2)

	elif workload == "rw":
		catalog = "parsec2/"
		granularity = "sampled_parsec/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		catalog = "parsec/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		loadrw(catalog, granularity)
		file_list = []
		catalog = "parsec2/"
		granularity = "samerw/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		catalog = "parsec/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		loadrw(catalog, granularity)
		file_list = []

		catalog = "cp/"
		granularity = "cp/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		loadrw(catalog, granularity)
		file_list = []
		catalog = "cp/"
		granularity = "samerw/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		loadrw(catalog, granularity)
		file_list = []

		catalog = "akamai/"
		granularity = "akamai/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		loadrw(catalog, granularity)
		file_list=[]
		catalog = "akamai/"
		granularity = "samerw/"
		load_simulation_file(root_dir, catalog, granularity, "online/")
		load_simulation_file(root_dir, catalog, granularity, "offline/")
		loadrw(catalog, granularity)

		handlerw()
	