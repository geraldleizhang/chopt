#include <iostream>
#include <algorithm>
#include <fstream>
#include <math.h>
#include <string>
#include <load.h>
#include <mcmf.h>
#include <time.h>
#include <omp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <mcmf_new.h>

using namespace std;

// DRAM NVM configuration assumption
// DRAM write read: 1 1
// NVM write read: 5 2
// SWAP would cause only write to new hardware


// #define DRAM_WRITE_SAVE -4
// #define DRAM_READ_SAVE -1
// #define SWAP_TO_DRAM_PENALTY 1
// #define SWAP_TO_NVM_PENALTY 5

#define DRAM_WRITE_SAVE -4
#define DRAM_READ_SAVE -4
#define SWAP_TO_DRAM_PENALTY 1
#define SWAP_TO_NVM_PENALTY 5


void Monitor(vector<int>& interval_list, 
	vector<bool>& option_list, 
	map<string, vector<int>>& item_history, 
	int k, 
	long long& cost, 
	string logfile, 
	bool isAcc = true,
	bool isDebug = true, 
	bool isOutput = false) {

	auto _size = interval_list.size();
	MCMF mcmf(3*_size+2, logfile, item_history, isAcc, isDebug, isOutput);
	// adding s
	mcmf.AddEdge(0,1,k,0);
	for(int i = 0; i < _size; i++) {
		if (interval_list[i] != INT_MAX) {
			if(option_list[i]) mcmf.AddEdge(_size+i+1, _size+interval_list[i]+1, 1, DRAM_WRITE_SAVE);
			else mcmf.AddEdge(_size+i+1, _size+interval_list[i]+1, 1, DRAM_READ_SAVE);	
		}

		if (i < _size - 1) {
			mcmf.AddEdge(i+1, i+2, INT_MAX, 0);
		}
		if (i < _size) {
			mcmf.AddEdge(i+1, i+1+_size, 1, SWAP_TO_DRAM_PENALTY);
			mcmf.AddEdge(i+1+_size, i+1+2*_size, 1, SWAP_TO_NVM_PENALTY);
			mcmf.AddEdge(i+1+2*_size, i+1, 1, 0);
		}
		
	}
	// adding t
	mcmf.AddEdge(_size, 3*_size+1, k, 0);
	
	mcmf.MincostMaxflow(0,3*_size+1, cost);
	return;
}

int main(int argc, char const *argv[])
{
	Load load;
	// string dir = "../data/";
	// string catalog = "sampling_test_cp2/";
	// string granularity = argv[1];
	string dir = "./";
	string catalog = "demo/";
	string granularity = "demo/";
	
	bool isOutput = true;
	bool isDebug = true;
	bool isAcc = true;

	string target_file = string(granularity).substr(0, string(granularity).size()-1);

	vector<string> _files;
	load.ReadDirectory( (dir+catalog).c_str(), _files);


	//_files.push_back("testTrace10");
	//_files.push_back("trace_radiosity.csv");
	if (catalog == "sampling_test_cp/" || catalog == "sampling_test_new/" || catalog == "sampling_test_akamai/" || catalog == "sampling_test_cp2/") {
		vector<string> _temp;
		for(auto it = _files.begin(); it != _files.end(); ++it) {
			size_t found = (*it).find_first_of("-");
			size_t found2 = (*it).find_last_of("-");
			string basename;
		  	if(found < 30) basename =  (*it).substr(0,found);
		  	else {
		  		size_t found2 = (*it).find_first_of(".");
		  		basename = (*it).substr(0, found2);
		  		//basename = "";
		  	}
		  	string sampling_rate = (*it).substr(found+1, found2-found-1);
		  	string sampling_order = (*it).substr(found2+1, -1);
			if(basename == target_file) _temp.push_back(*it);
			//if(*it == target_file + ".csv") _temp.push_back(*it);
		}
		_files.clear();
		for (auto it = _temp.begin(); it != _temp.end(); ++it) _files.push_back(*it);
	}

	sort(_files.begin(), _files.end());


	if(isAcc) {
		if(access( ("log_new/" + string(catalog)).c_str(), F_OK) != 0) 
			mkdir(("log_new/" + string(catalog)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(access( ("result/" + string(catalog)).c_str(), F_OK) != 0) 
			mkdir(("result/" + string(catalog)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(access( ("runtime/" + string(catalog)).c_str(), F_OK) != 0) 
			mkdir(("runtime/" + string(catalog)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

		if(access( ("log_new/" + string(catalog)+ granularity).c_str(), F_OK) != 0) 
			mkdir(("log_new/" + string(catalog)+ granularity).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(access( ("result/" + string(catalog)+ granularity).c_str(), F_OK) != 0) 
			mkdir(("result/" + string(catalog)+ granularity).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(access( ("runtime/" + string(catalog)+ granularity).c_str(), F_OK) != 0) 
			mkdir(("runtime/" + string(catalog)+ granularity).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	} else {
		if(access( ("log_appr/" + string(catalog)+ granularity).c_str(), F_OK) != 0) 
			mkdir(("log_appr/" + string(catalog)+ granularity).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(access( ("result_appr/" + string(catalog)+ granularity).c_str(), F_OK) != 0) 
			mkdir(("result_appr/" + string(catalog)+ granularity).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(access( ("runtime_appr/" + string(catalog)+ granularity).c_str(), F_OK) != 0) 
			mkdir(("runtime_appr/" + string(catalog)+ granularity).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	}


	if(catalog != "cp/") {
		omp_set_num_threads(_files.size());

		#pragma omp parallel for
		for(int i=0; i<_files.size(); i++) {
			time_t begin, end;
			time(&begin);

			auto _file = _files[i];
			auto filename = (dir + catalog + _file).c_str();

			vector<string> file;
			vector<bool> option_list;
			if (catalog == "cp/") load.LoadCPFile((dir + catalog + _file).c_str(), file, option_list);
			else load.LoadFile((dir + catalog + _file).c_str(), file, option_list);
			if (isDebug) {
				cout << "file loaded " << file.size() << endl;
			}
			vector<int> interval_list;
			unordered_map<string, int> frequency;
			map<string, vector<int>> item_history;
			load.LoadInterval(file, interval_list, frequency, item_history);
			int cachable_num = 0;
			int unique_num = load.UniqueNum(file, option_list, cachable_num);
			if (isDebug) {
				cout << "has " << unique_num << " unique items" << endl;
			}
			file.clear();
			frequency.clear();

			int k = unique_num;
			cout << _file << " " << unique_num << " " << cachable_num << endl;
			if(granularity.find("1000") == 0) {
				if(k > 1000) k = 1000;
			} 
			k = 2;			
			long long cost = 0;

			size_t found = (dir + catalog + granularity + _file).find_last_of("/");
		  	auto outfilename =  (dir + catalog + granularity + _file).substr(found+1);
		  	size_t found2 = outfilename.find_first_of(".");
		  	outfilename = outfilename.substr(0, found2);

			Monitor(interval_list, option_list, item_history, k, cost, catalog + granularity + outfilename, isAcc, true, isOutput);

			time(&end);
			#pragma omp critical
			{
				cout << _file << " done, length is " << interval_list.size() << " unique item is " << k << endl;
				cout << "cost is " << cost << endl;
			
				cout << "time takes " << (end-begin) << "s" << endl;
			}
			
			interval_list.clear();
			option_list.clear();

		}
	}
	else if (catalog == "cp/") {

		/*re-order all the files with the real cachable item num*/
		map<int, vector<string>> temp;
		#pragma omp parallel for
		for(int i=0; i<_files.size(); i++ ){
			auto _file = _files[_files.size()-i-1];
			auto filename = (dir + catalog + _file).c_str();
			Load _load;
			vector<string> file;
			vector<bool> option_list;
			_load.LoadCPFile(filename, file, option_list);
			int cachable_num = 0;
			int unique_num = _load.UniqueNum(file, option_list, cachable_num);
			temp[cachable_num].push_back(_file);
			file.clear();
			option_list.clear();
		}
		_files.clear();

		for(auto it = temp.begin(); it != temp.end(); ++it) {
			for(auto iit = it->second.begin(); iit != it->second.end(); ++iit){
				_files.push_back(*iit);
			} 
		}

		int range[4] = {0, 40, 80, 106}; 
		for(int j=1; j<2; j++) {
			int round_start = range[j];
			int round_end = range[j+1];

			omp_set_num_threads(round_end - round_start);
			
			#pragma omp parallel for
			for (int i = round_start; i < round_end ; ++i) {
				time_t begin, end;
				time(&begin);

				auto _file = _files[i];
				//string _file = "w86_vscsi1.vscsitrace.sample";
				auto filename = (dir + catalog + _file).c_str();

				vector<string> file;
				vector<bool> option_list;
				if (catalog == "cp/") load.LoadCPFile(filename, file, option_list);
				else load.LoadFile(filename, file, option_list);
				if (isDebug) {
					cout << _file << " file loaded" << endl;
				}
				vector<int> interval_list;
				unordered_map<string, int> frequency;
				map<string, vector<int>> item_history;
				load.LoadInterval(file, interval_list, frequency, item_history);
				int cachable_num = 0;
				int unique_num = load.UniqueNum(file, option_list, cachable_num);
				if (isDebug) {
					cout << "has " << unique_num << " unique items, where " << cachable_num << " are cachable." << endl;
				}
				file.clear();
				frequency.clear();

				int k = unique_num;
				if(granularity.find("1000") == 0) {
					if(k > 1000) k = 1000;
				}
				cout << _file << " " << unique_num << " " << cachable_num << endl;
				long long cost = 0;

				size_t found = (dir + catalog + granularity + _file).find_last_of("/");
			  	auto outfilename =  (dir + catalog + granularity + _file).substr(found+1);
			  	size_t found2 = outfilename.find_first_of(".");
			  	outfilename = outfilename.substr(0, found2);

				Monitor(interval_list, option_list, item_history, k, cost, catalog + granularity + outfilename, isAcc, true, isOutput);

				time(&end);
				#pragma omp critical
				{
					cout << _file << " done, length is " << interval_list.size() << " unique item is " << k << endl;
					//cout << interval_list.size() << " " << k << endl;
					cout << "cost is " << cost << endl;
				
					cout << "time takes " << (end-begin) << "s" << endl;
				}
				
				interval_list.clear();
				option_list.clear();
			}
		}
	}
	
	return 0;
}
