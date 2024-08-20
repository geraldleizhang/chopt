import os
import random

file_list = []

def shuffle(data):
	shuffled_data = data.copy()
	for i in range(len(shuffled_data)-1, 0, -1):
		_index = random.randint(0, i)
		temp = shuffled_data[i]
		shuffled_data[i] = shuffled_data[_index]
		shuffled_data[_index] = temp
	return shuffled_data
	

def load_file():
	root_dir = "../data/sampled/short/"
	temp = os.listdir(root_dir)
	#temp = ["trace_fft.csv"]
	for item in temp:
		file_list.append(item)

	for fname in file_list:
		file = open(root_dir+fname)
		data = file.readlines()
		for i in range(10):
			shuffled_data = shuffle(data)
			f = open("../data/sampled/short/shuffled/"+fname.split(".csv")[0]+"_"+str(i)+".csv", 'w')
			f.writelines(shuffled_data)
			f.close()
		
if __name__ == '__main__':
	load_file()
	#a=[1,2,3,4,5,6,7,8,9,10]
	#for i in range(5):
	#	shuffle(a)