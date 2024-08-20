import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

if __name__ == '__main__':
	catalog='short'
	with open('plot/'+catalog) as f:
		filename=""
		length=0
		unique_num=0
		cache_size=0
		opt_cost=0
		lru_cost=0
		file_list=[]
		length_list=[]
		unique_num_list=[]
		cache_size_list={}
		opt_cost_list={}
		lru_cost_list={}
		while True:
			line = f.readline()
			if not line:
				break
			
			items = line.split("\n")[0].split(" ")
			
			if(items[0] != "cache"):
				filename = items[0]
				length = int(items[2])
				unique_num = int(items[5])
				file_list.append(filename)
				length_list.append(length)
				unique_num_list.append(unique_num)
				cache_size_list[filename]=[]
				opt_cost_list[filename]=[]
				lru_cost_list[filename]=[]
			else:
				cache_size=int(items[2])
				opt_cost=int(items[5])
				lru_cost=int(items[8])
				cache_size_list[filename].append(cache_size)
				opt_cost_list[filename].append(opt_cost)
				lru_cost_list[filename].append(lru_cost)


	plt.figure(figsize=(20,8))
	for i in range(1,len(file_list)):
		item=file_list[i]
		length = length_list[i]
		unique_num = unique_num_list[i]
		c=np.asarray(cache_size_list[item])
		o=np.asarray(opt_cost_list[item])
		l=np.asarray(lru_cost_list[item])

		plt.subplot(2, len(file_list)-1, i)
		plt.ylim(0, 11)
		plt.xlabel("cache size")
		plt.plot(c, o/length, label="opt")
		plt.plot(c, l/length, label="lru")
		plt.hlines(5, c[0], c[-1], colors='grey', alpha=0.5, linestyles='dashed')
		plt.hlines(10, c[0], c[-1], colors='grey', alpha=0.5, linestyles='dashed')
		plt.legend()
		if i == 1:
			plt.ylabel("avg cost per access")
		plt.title(item)

		plt.subplot(2, len(file_list)-1, i+len(file_list)-1)
		#plt.ylim(0, 10)
		plt.plot(c, o/unique_num, label="opt")
		plt.plot(c, l/unique_num, label="lru")
		plt.xlabel("cache size")
		#plt.hlines(5, c[0], c[-1], colors='grey', alpha=0.5, linestyles='dashed')
		plt.hlines(10, c[0], c[-1], colors='grey', alpha=0.5, linestyles='dashed')
		plt.legend()
		if i == 1:
			plt.ylabel("avg cost per cache item")


	plt.savefig("plot/"+catalog+".png")