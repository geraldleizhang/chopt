import os
import sys
import numpy as np
import multiprocessing as mp
import operator
import pandas as pd

catalogs = ["parsec2/","parsec/","parsec2/", "cp/","cp/","cp/", "akamai/","akamai/","akamai/"]
files = ["trace_streamcluster", "trace_radiosity", "trace_graph500_s25_e25", "w06_vscsi1", "w09_vscsi1", "w13_vscsi1", "lax_1448_2", "lax_1448_3", "lax_1448_5"]




def get_cdf(data):
	unique, counts = np.unique(data, return_counts=True)
	for i in range(1,len(counts)):
		counts[i] += counts[i-1]
	return unique, counts/counts[-1]

def plot_rd():
	file_dir = "../data/rd/"
	for i in range(9):
		out = open("gnuplot_result/rd_"+files[i] + ".dat", "w")
		print(catalogs[i], files[i])
		_rd = np.asarray(pd.read_csv(file_dir + catalogs[i] + files[i], header=None, sep=" ")).astype(int)
		x, y = get_cdf(_rd)
		for j in range(1, x.shape[0]):
			out.write(str(x[j]+1) + " " + str(y[j]) + "\n")
		out.close()
	return


def plot_sd():
	file_dir = "../data/age/"
	for i in range(9):
		out = open("gnuplot_result/sd_"+files[i] + ".dat", "w")
		print(catalogs[i], files[i])
		_sd = np.asarray(pd.read_csv(file_dir + catalogs[i] + files[i], header=None, sep=" ")).astype(int)
		x, y = get_cdf(_sd)
		for j in range(1, x.shape[0]):
			out.write(str(x[j]+1) + " " + str(y[j]) + "\n")
		out.close()
	return

def plot_pd():
	file_dir = "../data/popularity/"
	for i in range(9):
		out = open("gnuplot_result/pd_"+files[i] + ".dat", "w")
		print(catalogs[i], files[i])
		_pd = np.flip(np.asarray(pd.read_csv(file_dir + catalogs[i] + files[i], header=None, sep=" "))[:,1].astype(int))
		for j in range(_pd.shape[0]):
			out.write(str(j+1) + " " + str(_pd[j]) + "\n")
		out.close()
	return

if __name__ == '__main__':
	s_type = sys.argv[1]

	if s_type == "rd":
		plot_rd()
	elif s_type == "sd":
		plot_sd()
	elif s_type == "pd":
		plot_pd()