#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <queue>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <algorithm>
#include <pthread.h>
#include <unistd.h>
#include <load.h>
#include <omp.h>
#include <time.h>
#ifndef __ANALYSIS_H__
#define __ANALYSIS_H__
#include <monitor.h>


#endif

using namespace std;

const char* dir_as_char(const char* a, const char* b) {
	return (string(a)+string(b)).c_str();
}

const char* dir_as_char(const char* a, const char* b, const char* c) {
	return (string(a)+string(b)+string(c)).c_str();
}

string dir_as_string(const char* a, const char* b) {
	return string(a)+string(b);
}

string dir_as_string(const char* a, const char* b, const char* c) {
	return string(a)+string(b)+string(c);
}


struct _log
{
	int cap;
	int flow;
	int cost;
};

// log: read from log file to reconstruct cache result
// need to define cache size

void log(string dir, 
	string filename, 
	vector<int> cache_size, 
	map<int, vector<int>>& swap_in, 
	map<int, vector<int>>& swap_out,
	int node_num) {
	ifstream in, in_info;
	in_info.open((dir+filename+"_info").c_str());
	//in.open((dir+filename).c_str());

	map<pair<int, int>, int> cost_list;

	int num_items = 0;
	in_info >> num_items;

	vector<int> visited_num;
	int from=0, to=0, cap=0, flow=0, cost=0, round=0, visited=0;
	while(true) {
		in_info >> round >> visited >> flow >> cost;
		if(in_info.eof()) break;
		visited_num.push_back(visited);
	}
	in_info.close();

	// cout << filename << " info done" << endl;

	int curr_index = 0;
	int curr_cache_size = cache_size[curr_index];

	Load load;
	char* data;
	long size;
	load.LoadMemory((dir+filename).c_str(), data, size);

	// input stage
	// 0:round
	// 1:from
	// 2:to
	// 3:cap
	// 4:flow
	// 5:cost
	int stage = 0;
	int input;
	int num = 0;
	bool num_ready=false;
	int nega = 1;
	int curr_round;
	bool end_flag = false;
	char item;

	for(long i=0; i<size; i++) {		
		item = data[i];
		
		if(item == '-') {
			nega = -1;
			num_ready = false;
		}
		else if( item == ' ' || item == '\n' ) {
			input = nega * num;
			nega = 1;
			num_ready = true;
			num=0;
		} else {
			int temp = item - '0';
			num = num*10+temp;
			num_ready = false;
		}

		if(!num_ready) continue;

		// we have an input here
		switch(stage){
			case 0:
				round = input;
				if(round == cache_size[cache_size.size()-1]) {
					end_flag=true;
					break;
				}
				stage=1;
				curr_round = visited_num[round];
				break;
			case 1:
				from = input-1;
				stage=2;

				break;
			case 2:
				to = input-1;
				stage=3;
				break;
			case 3:
				cap = input;
				stage=4;
				break;
			case 4:
				flow = input;
				stage=5;
				break;
			case 5:
				cost = input;
				if((to-from) % num_items == 0 || (from-to) % num_items == 0) {
					//if(from > to) cout << from << " " << to << endl;
					if (flow > 0 ) {
						auto temp = make_pair(from, to);
						cost_list[temp] = 1;
					} else if (flow == 0) {
						auto temp = make_pair(from, to);
						cost_list[temp] = 0;
					}
				}

				curr_round--;
				if(!curr_round){
					if(round == curr_cache_size-1) {
						
						for(auto it = cost_list.begin(); it != cost_list.end(); ++it) {
							if (it->second > 0) {
								if (it->first.first > it->first.second){
									swap_out[curr_cache_size].push_back(it->first.second);
								}
								else if (it->first.first < num_items && it->first.second < 2*num_items) {
									swap_in[curr_cache_size].push_back(it->first.first);
								}
							}
						}
						curr_index++;
						curr_cache_size = cache_size[curr_index];
					}
					stage = 0;
				} 
				else stage = 1;
				break;
		}
	
		if(end_flag) break;
	}
	
	//in.close();
	
	cost_list.clear();
	return;
}

int log_new(string dir, 
	string filename, 
	vector<int> cache_size, 
	map<int, vector<int>>& swap_in, 
	map<int, vector<int>>& swap_out) {
	
	Load load;
	auto unique_item = load.LoadLog((dir+filename).c_str(), cache_size, swap_in, swap_out);

	return unique_item;
}


// result: read from result file to reconstruct cache result
void result(string dir, string filename, vector<int>& swap_in, vector<int>& swap_out) {
	ifstream in;
	in.open((dir+filename).c_str());
	
	map<pair<int, int>, int> cost_list;

	int num_items=0, from=0, to=0, cap=0, flow=0, cost=0;
	in >> num_items;
	if(in.eof()) {
		in.close();
		return;
	}
	while(true) {
		in >> from >> to >> cap >> flow >> cost;
		if(in.eof()) break;
		from--;
		to--;

		if (cost > 0) {
			if (from > to) swap_out.push_back(to);
			else swap_in.push_back(from);
		}
		auto temp = make_pair(from, to);
		cost_list[temp] = cost;
	}
	in.close();

	cost_list.clear();
	return;
}




struct res_vector{
	int time;
	int isWrite; 	// global
	int rd; 	 	// global
	int age; 		// global
	int* frequency; // global
	int isScan; 	// global
	int isCached; 	// specific to cache size
};

bool isInScan(int i, vector<pair<int,int>> scans) {
	if(!scans.size()) return false;
	for (auto it = scans.begin(); it != scans.end(); ++it) {
		int s = (*it).first;
		int t = (*it).second;
		if(i >= s && i <= t) return true;
		if(i < s) return false;
	}
	return false;
}

// analyze global behavior, including:
// 		1. scans
void analyze_trace_behavior(vector<string>& file, 
	vector<bool>& option_list, 
	vector<bool>& scans,
	int unique_num) {
	map<string, int> scan_counter;
	int scan_round = 1;
	int curr_round_starter = 0;

	for (int i=0; i<file.size(); i++) {
		auto item = file[i];
		 
		// detect scan:
		// if item in scan_counter:
		//  	it means the item happened again in the current round, so push the current round to scans, and move to the next round
		// if not, mark the scan_counter[i] as the current round
		if(scan_counter.find(item) != scan_counter.end()) {
			if(i-1 - curr_round_starter > (unique_num / 4)) {
				//auto temp = make_pair(curr_round_starter, i-1);
				//scans.push_back(temp);
				for(int j=curr_round_starter; j<i-1;j++) scans[j]=true;
			}
			
			curr_round_starter = i;
			scan_round++;
			scan_counter.clear();
		} else {
			scan_counter[item] = 1;
		}
		scans.push_back(false);
	}
	scan_counter.clear();
	return;
}

// analyze each item's activity
// for each indicator, we calculate it per item and form an array as a distribution of the indicator
// output:
// file
// 	 for each item: [time, r/w, rd, age, frequency[w1, w2, w3], isScan]
// map<string, map<string, vector<>>>

void analyze_per_item_activity(
	map<string, vector<pair<int, int>>>& item_history, 
	map<string, vector<pair<int, int>>>& rd, 
	vector<bool>& option_list, 
	int cache_size, 
	int windows[3], 
	vector<pair<int, int>> scans, 
	map<string, vector<res_vector*>>& res_matrix, 
	bool isRd) {
	// there are always two dimensions: read/write, and cached/not cached, for each indicator
	int read_count = 0;
	int write_count = 0;

	int cached_count = 0;
	int non_cached_count = 0;

	int item_num = item_history.size();

	for (auto iit = item_history.begin(); iit != item_history.end(); ++iit) {
		auto item = iit->first;
		auto history = iit->second;

		//time: operation
		for (int i = 0; i < history.size(); i++) {
			auto it = history[i];
			if(option_list[it.first]) write_count++;
			else read_count++;
			if(it.second) cached_count++;
			else non_cached_count++; 

			//if(!it.second) {
				// if an item is chosen not to be cached, count several items:
				// 1. read/write
				// 2. rd (age)
				// 3. frequency, given 3 different windows
				// 4. is a scan
				int age = 0;
				int* freq = new int[3]{0,0,0};
				int curr = i-1;
				while(curr>=0) {
					int diff = history[i].first - history[curr].first;
					if(curr == i-1) age = diff;
					if(diff <= windows[0]) freq[0]++;
					if(diff <= windows[1]) freq[1]++;
					if(diff <= windows[2]) freq[2]++;
					else break;
					curr--;
				}

				res_vector* temp = new res_vector{it.first, option_list[it.first], isRd? rd[item][i].second : 0, age, freq, isInScan(i, scans), it.second};
				res_matrix[item].push_back(temp);
			//}
		}
	}
	return;
}

void calc_next_access_list(
	vector<string>& file,
	vector<int>& next_access_list) {
	
	map<string, vector<int>> item_history;
	item_history.clear();
	for (int i=0; i<file.size(); i++) {
		string item = file[i];
		item_history[item].push_back(i);
		next_access_list.push_back(INT_MAX);
	}
	
	int count = 0;
	for (auto it = item_history.begin(); it != item_history.end(); ++it) {
		//count++;
		//cout << count << " / " << item_history.size() << endl;
		auto history = it->second;
		//#pragma omp parallel for
		for(int i=0; i<history.size(); i++) {
			auto index = history[i];
			if(i != history.size() - 1) {
				next_access_list[index] = history[i+1];
			}
		}
	}
	return;
}

void calc_item_history(
	vector<string>& file,
	map<string, vector<int>>& item_history,
	vector<int>& next_access_list,
	vector<int>& age,
	vector<int>& windows,
	map<int, vector<int>>& frequency,
	map<string, int>& all_frequency ) {
	
	for (int i=0; i<file.size(); i++) {
		string item = file[i];
		item_history[item].push_back(i);
		next_access_list.push_back(INT_MAX);
		for(int j=0; j<windows.size(); j++) frequency[i].push_back(0);
	}
	
	//#pragma omp parallel for
	//int _count = 0;
	for (auto it = item_history.begin(); it != item_history.end(); ++it) {
		//cout << _count << " / " << item_history.size() << endl;
		//_count++;
		auto history = it->second;
		all_frequency[it->first] = history.size();
		//sort(history.begin(), history.end());
		//#pragma omp parallel for
		for(int i=0; i<history.size(); i++) {
		
			auto index = history[i];

			if(i==0) {
				age[index] = -1;
				for (int w=0; w<windows.size(); w++) frequency[index][w] = 0;
			} else {
				int curr = i-1;
				age[index] = history[i] - history[i-1]-1;
				while(curr >= 0) {
					int diff = history[i] - history[curr] - 1;
					// age[index] = diff;
					if(diff > windows[windows.size()-1]) break;
					if(diff <= windows[0]) frequency[index][0]++;
					for (int w=1; w<windows.size(); w++) {
						if (diff <= windows[w] && diff > windows[w-1]) {
							frequency[index][w]++;
						}
					}
					curr--;
				}
			}

			if(i != history.size() - 1) {
				next_access_list[index] = history[i+1];
			}
		}
	}
	


	return;
}


// for cache size, choose at most the previous 6 different sizes
// modified: choose 10 continuous cache size
// for window size, choose 4 previous, and 4 next 
void load_cache_and_window_list(string dir, 
	string filename,
	int length,
	vector<int>& cache_size_list,
	vector<int>& windows) {
	
	ifstream in_info;
	in_info.open((dir+filename+"_info").c_str());
	int num_items = 0;
	in_info >> num_items;

	vector<int> visited_num;
	int flow=0, cost=0, round=0, visited=0;
	while(true) {
		in_info >> round >> visited >> flow >> cost;
		if(in_info.eof()) break;
	}
	in_info.close();

	int unique_num = round + 1;

	if(unique_num < 30) {
		for(int i=2; i<unique_num; i+=2) cache_size_list.push_back(i);
	} else {
		cache_size_list.emplace(cache_size_list.begin(), unique_num);
		//windows.emplace(windows.begin(), unique_num);
		
		int closest_size = 2;
		while(closest_size < unique_num) closest_size *= 2;
		auto temp = closest_size / 2;
		while(temp > 2) {
			cache_size_list.emplace(cache_size_list.begin(), temp);
			//windows.emplace(windows.begin(), temp);
			temp /= 2;
			if(cache_size_list.size() > 8) break;
		}
	}

	

	/*temp = closest_size;
	while(temp < length) {
		windows.push_back(temp);
		temp *= 2;
		if(windows.size() > 9) break;
	}*/
	for(int i=1; i<=10; i++) {
		windows.push_back(1000*i);
	}

	return;
}


void load_sampling_cache(string dir, 
	string filename,
	vector<int>& cache_size_list,
	vector<int>& windows) {
	
	ifstream in_info;
	in_info.open((dir+filename+"_info").c_str());
	int num_items = 0;
	in_info >> num_items;

	vector<int> visited_num;
	int flow=0, cost=0, round=0, visited=0;
	while(true) {
		in_info >> round >> visited >> flow >> cost;
		if(in_info.eof()) break;
	}
	in_info.close();

	int unique_num = round + 1;

	if(unique_num < 20) for(int i=2; i<unique_num; i+=2) cache_size_list.push_back(i); 

	else for(int i = 4; i < unique_num; i*=2) cache_size_list.push_back(i);
	//cache_size_list.push_back(unique_num);



	for(int i=1; i<=10; i++) {
		windows.push_back(1000*i);
	}

	return;
}

void print_vector_int(vector<int> v) {
	for (auto it = v.begin(); it != v.end(); ++it) cout << *it << " ";
	cout << endl;
	return;
}

// Analysis
// file[access]
// option_list[access]
// scans[access]
// rd[access]
// age[access]
// frequency[access, windows]
// result[cache_size, access]
void handle_analysis(const char* result_dir, 
	const char* log_dir, 
	const char* rd_dir,
	const char* source_dir, 
	const char* catalog,
	const char* granularity,
	bool isOnline, 
	bool isCustom = false,
	bool isResult = false, 
	bool isOutput = false,
	bool isDebug = false) {

	bool isSameRW = false;
	if (granularity == "samerw/") isSameRW = true;



	string anal_type = isOnline? "online/" : "offline/";
	cout << anal_type << endl;

	if(isOutput) {
		if(access( ("simulation/" + string(catalog)).c_str(), F_OK) != 0) 
			mkdir(("simulation/" + string(catalog)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(access( ("simulation/" + string(catalog)+ string(granularity)).c_str(), F_OK) != 0) 
			mkdir(("simulation/" + string(catalog)+ string(granularity)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(access( ("simulation/" + string(catalog)+ string(granularity) + anal_type).c_str(), F_OK) != 0) 
			mkdir(("simulation/" + string(catalog)+ string(granularity) + anal_type).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(access( ("simulation/" + string(catalog)+ string(granularity) + "custom").c_str(), F_OK) != 0) 
			mkdir(("simulation/" + string(catalog)+ string(granularity) + "custom").c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(access( ("cache_behavior/" + string(catalog)).c_str(), F_OK) != 0) 
			mkdir(("cache_behavior/" + string(catalog)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(access( ("cache_behavior/" + string(catalog)+ string(granularity)).c_str(), F_OK) != 0) 
			mkdir(("cache_behavior/" + string(catalog)+ string(granularity)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(access( ("cache_behavior/" + string(catalog)+ string(granularity) + anal_type).c_str(), F_OK) != 0) 
			mkdir(("cache_behavior/" + string(catalog)+ string(granularity) + anal_type).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);		
		if(access( ("chopt/" + string(catalog)).c_str(), F_OK) != 0) 
			mkdir(("chopt/" + string(catalog)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(access( ("chopt/" + string(catalog)+ string(granularity)).c_str(), F_OK) != 0) 
			mkdir(("chopt/" + string(catalog)+ string(granularity)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);		
	}

	vector<string> result_files;

	// cout << dir_as_char(log_dir, catalog, granularity) << endl;

	Load _load;
	//_load.ReadDirectory(dir_as_char(log_dir, catalog, granularity),result_files);
	_load.ReadDirectory((string(log_dir) + string(catalog) + string(granularity)).c_str(),result_files);


	// if(catalog == "short/") {
	// 	for(auto it = result_files.begin(); it != result_files.end(); it++) {
	// 		if ( (*it) == "trace_canneal" || "trace_radiosity" ) result_files.erase(it);
	// 		if ( (*it) == "1000" ) result_files.erase(it);
	// 		if ( (*it) == "max" ) result_files.erase(it);
	// 	}
	// } 

	// else if (catalog == "parsec/" || catalog == "parsec2/") {
	// 	for(auto it = result_files.begin(); it != result_files.end(); it++) {
	// 		if ( (*it) == "trace_radix" || "trace_fft" ) result_files.erase(it);
	// 		//if ( (*it) == "1000" ) result_files.erase(it);
	// 		//if ( (*it) == "max" ) result_files.erase(it);
	// 	}
	// }

	sort(result_files.begin(), result_files.end());
	// for (auto it = result_files.begin(); it != result_files.end(); ++it) cout << *it << endl;
    // result_files.push_back("trace_x264");
    // result_files.push_back("trace_bodytrack");
    // result_files.push_back("trace_lu_ncb");
    // result_files.push_back("trace_blackscholes");
    // result_files.push_back("trace_radiosity");
    // result_files.push_back("trace_barnes");
    // result_files.push_back("trace_radix");
    // result_files.push_back("trace_fft");
    
    // result_files.push_back("trace_streamcluster");
    // result_files.push_back("trace_vips");
    // result_files.push_back("trace_fluidanimate");
    // result_files.push_back("trace_ocean_ncp");
    // result_files.push_back("trace_freqmine");
    // result_files.push_back("trace_canneal");
    // result_files.push_back("trace_graph500_s25_e25");

	// result_files.push_back("lax_1448_0");
	// result_files.push_back("lax_1448_1");
	// result_files.push_back("lax_1448_2");

	int range[6] = {0,20,40,60,80,106};
	int end_loop = 4;

	if(catalog != "cp/") {
		range[1] = result_files.size();
		end_loop = 1;
	} else {
		map<int, vector<string>> temp;
		#pragma omp parallel for
		for(int i=0; i<result_files.size(); i++ ){
			auto _file = result_files[result_files.size()-i-1];
			auto filename = (dir_as_string(source_dir, catalog) + _file + ".vscsitrace.sample").c_str();
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
		result_files.clear();

		for(auto it = temp.begin(); it != temp.end(); ++it) {
			for(auto iit = it->second.begin(); iit != it->second.end(); ++iit){
				result_files.push_back(*iit);				
			} 
			cout << endl;
		}
	}


	for (int l=0; l<end_loop; l++) {
		int round_start = range[l];
		int round_end = range[l+1];
		//int round_start = 0;
		//int round_end = result_files.size();
		if(isDebug) cout << "current round " << round_start << " " << round_end << endl;
		//omp_set_num_threads(round_end - round_start);
		//#pragma omp parallel for

		for(int i=round_start; i< round_end; i++) {
			// output file
			ofstream out;
			ofstream behavior_out;

			if(isDebug) cout << "file " << result_files[i] << endl;

			// offline: chopt, belady, static, beladyac
			// online: lru, lfu, tinylfu, W-tinylfu

			Load load;
			// original file
			vector<string> file;
			// read/write
			vector<bool> option_list;

			if(string(catalog) == "cp/") 
				load.LoadCPFile((dir_as_string(source_dir, catalog) + result_files[i] + ".vscsitrace.sample").c_str(), file, option_list);
			else if(string(catalog) == "akamai/" || string(catalog) == "test/") 
				load.LoadFile((dir_as_string(source_dir, catalog) + result_files[i]).c_str(), file, option_list);
			else 
				load.LoadFile((dir_as_string(source_dir, catalog) + result_files[i] + ".csv").c_str(), file, option_list);
			int unique_num = load.UniqueNum(file);
			if (file.size() == 0) continue;

			if(isDebug) cout << result_files[i] << " " << unique_num << " " << file.size() << endl;

			int cache_size_temp = 0;
			vector<int> cache_size_list;
			vector<int> windows;

			load_cache_and_window_list(dir_as_string(log_dir, catalog, granularity), result_files[i], file.size(), cache_size_list, windows);
			if(isDebug) cout << "cache and window list loaded" << endl;
			// chopt, belady, static, beladyac
			if(!isOnline) {
				// detect scans in a trace
				vector<bool> scans;
				analyze_trace_behavior(file, option_list, scans, unique_num);
				// history per item
				map<string, vector<int>> item_history;
				// next access list for each item
				vector<int> next_access_list;
				// rd
				vector<int> rd;
				Load _load;
				_load.LoadRd( (dir_as_string(rd_dir, catalog) + result_files[i]).c_str(), rd );
				// age
				vector<int> age;
				for(int j=0; j<file.size(); j++) age.push_back(0);
				// frequency, given windows
				map<int, vector<int>> frequency;
				// overall frequency, for each item
				map<string, int> all_frequency;

				if(!isCustom)
				// calculate age and frequency
				calc_item_history(file, item_history, next_access_list, age, windows, frequency, all_frequency);



				// output frequency to data/frequency/catalog/filename/cache_size. only output when the file is not exist
				if(isOutput) {
					string frequency_dir = "../data/frequency/"; 
					for(int j=0; j<windows.size(); j++) {
						
						auto window_size = windows[j];
						if(access( (frequency_dir+ string(catalog)).c_str(), F_OK) != 0) mkdir((frequency_dir+ string(catalog)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
						if(access( (frequency_dir+ string(catalog)+ result_files[i]).c_str(), F_OK) != 0) mkdir((frequency_dir+ string(catalog)+ result_files[i]).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
						if(access( (frequency_dir + string(catalog) + result_files[i] + "/" + to_string(window_size)).c_str(), F_OK) != 0) {
							if(isDebug) cout << "start loading " << result_files[i] << " " << window_size << " frequency" << endl;
							ofstream frequency_out;
							frequency_out.open(frequency_dir + string(catalog) + result_files[i] + "/" + to_string(window_size), ios::out | ios::trunc);
							for(int k=0; k<file.size(); k++) frequency_out << frequency[k][j] << endl;
							frequency_out.close();
							if(isDebug) cout << result_files[i] << " " << window_size << " frequency loaded" << endl;
						}
					}
				}

				// output age to data/age/catalog/filename. only output when the file is not exist
				if(isOutput) {
					string age_dir = "../data/age/"; 
					if(access( (age_dir+ string(catalog)).c_str(), F_OK) != 0) mkdir((age_dir+ string(catalog)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
					if(access( (age_dir + string(catalog) + result_files[i]).c_str(), F_OK) != 0) {
						if(isDebug) cout << "start loading age" << endl;
						ofstream age_out;
						age_out.open(age_dir + string(catalog) + result_files[i], ios::out | ios::trunc);
						for(int k=0; k<age.size(); k++) age_out << age[k] << endl;
						age_out.close();
						if(isDebug) cout << result_files[i] << " age loaded" << endl;
					} 
				}

				if(isDebug) cout << "item history calculated" << endl;
				continue;

				if(isDebug) {
					cout << "cache size" << endl;
					for(auto it = cache_size_list.begin(); it !=cache_size_list.end(); ++it) cout << *it << " ";
					cout << endl;
					cout << "windows size" << endl;
					for(auto it = windows.begin(); it !=windows.end(); ++it) cout << *it << " ";
					cout << endl;
				}

				// opt cache swap in/out result
				map<int, vector<int>> swap_in;
				map<int, vector<int>> swap_out;

				if(!isCustom) {
					// for all cache sizes, calculate swap in/out
					if(isDebug) cout << "log file beginning" << endl;
					//log(dir_as_string(log_dir, catalog, granularity), result_files[i], cache_size_list, swap_in, swap_out, 2*file.size()+2);
					log_new(dir_as_string(log_dir, catalog, granularity), result_files[i], cache_size_list, swap_in, swap_out);
					if(isDebug) cout << "log file loaded" << endl;
					cout << swap_in.size() << " " << swap_out.size() << endl;
				}

				// cache result
				map<int, vector<int>> chopt_result;
				map<int, vector<int>> belady_result;
				map<int, vector<int>> beladyac_result;
				map<int, vector<int>> static_result;
				map<int, vector<int>> lr_spatial_result;
				map<int, vector<int>> lr_temporal_result;

				// opt and lru cache cost		
				vector<int> opt_cost_list;
				vector<int> belady_cost_list;
				vector<int> beladyac_cost_list;
				vector<int> static_cost_list;
				vector<int> lr_spatial_cost_list;
				vector<int> lr_temporal_cost_list;

				for(int j=0; j<cache_size_list.size(); j++) {
					opt_cost_list.push_back(0);
					belady_cost_list.push_back(0);
					beladyac_cost_list.push_back(0);
					static_cost_list.push_back(0);
					lr_spatial_cost_list.push_back(0);
					lr_temporal_cost_list.push_back(0);
				}

				string lr_types[2] = {"freq/", "all/"};
				omp_set_num_threads(40);
				#pragma omp parallel for
				for(int k=0; k<7*cache_size_list.size(); k++) {
					auto j = k / 7;
					auto cache_option = k % 7;

					if(isCustom && cache_option < 3) continue;

					if(!isCustom && cache_option > 2) continue;

					int cache_size = cache_size_list[j];
					int cost=0;
					if(!isCustom) {
						if(swap_in[cache_size].size() == 0 || swap_out[cache_size].size() == 0) continue; 
					}

					//if((swap_in[cache_size].size() && swap_out[cache_size].size())  || isCustom) {
						vector<int> chopt_res;
						vector<int> belady_res;
						vector<int> beladyac_res;
						vector<int> static_res;
						vector<int> lr_spatial_res;
						vector<int> lr_temporal_res;
						string lr_type = lr_types[(cache_option + 1)%2];
						
						switch(cache_option) {
							case 0:
								cost = chopt_monitor(file, option_list, swap_in[cache_size], swap_out[cache_size], chopt_res, isSameRW);
								opt_cost_list[j] = cost;
								chopt_result[cache_size] = chopt_res;
								break;
							case 1:	
								
								cost = belady_monitor(file, option_list, next_access_list, cache_size, belady_res, isSameRW);
								belady_cost_list[j] = cost;
								belady_result[cache_size] = belady_res;
								break;
							case 2:	
								
								// cost = static_monitor(file, option_list, all_frequency, cache_size, static_res, isSameRW);
								// static_cost_list[j] = cost;
								// static_result[cache_size] = static_res;
								cost = beladyac_monitor(file, option_list, next_access_list, cache_size, beladyac_res, isSameRW);
								beladyac_cost_list[j] = cost;
								beladyac_result[cache_size] = beladyac_res;
								break;
							case 3:
							case 5:	
								{
									Load offline_load;
									vector<int> operation;
									offline_load.LoadOffline( ("linear_regression/" + dir_as_string(catalog, granularity) + lr_type + result_files[i] + "/" + "spatial/" + to_string(cache_size)).c_str() ,operation);
									cost = offline_monitor(file, option_list, operation, cache_size, lr_spatial_res);
									lr_spatial_cost_list[j] = cost;
									lr_spatial_result[cache_size] = lr_spatial_res;
									operation.clear();
									break;
								}
							case 4:	
							case 6:
								{
									Load offline_load;
									vector<int> operation;
									offline_load.LoadOffline( ("linear_regression/" + dir_as_string(catalog, granularity) + lr_type + result_files[i] + "/" + "temporal/" + to_string(cache_size)).c_str() ,operation);
									cost = offline_monitor(file, option_list, operation, cache_size, lr_temporal_res);
									lr_temporal_cost_list[j] = cost;
									lr_temporal_result[cache_size] = lr_temporal_res;
									operation.clear();	
									break;
								}
						}
					//}

					#pragma omp critical 
					{
						if(isDebug) cout << cache_size << " " << cache_option << " done" << endl;
					}
				}

				#pragma omp critical
				{
					cout << result_files[i] << endl;
					for(int j=0; j<cache_size_list.size(); j++) {
						cout << cache_size_list[j] << " " << opt_cost_list[j] << " " << belady_cost_list[j] << " " << beladyac_cost_list[j] << " " << lr_spatial_cost_list[j] << " " << lr_temporal_cost_list[j] << endl; 
					}
				}
				


				scans.clear();
				for(auto it = item_history.begin(); it != item_history.end(); ++it) it->second.clear();
				item_history.clear();
				rd.clear();
				age.clear();
				for(auto it = frequency.begin(); it != frequency.end(); ++it) it->second.clear();
				frequency.clear();
				
				swap_in.clear();
				swap_out.clear();


				if(isOutput && !isCustom) {
					out.open("simulation/" + string(catalog) + string(granularity) + anal_type + result_files[i], ios::out | ios::trunc);
					behavior_out.open("cache_behavior/" + string(catalog) + string(granularity) + anal_type + result_files[i], ios::out | ios::trunc);
					//ofstream chopt_out;
					//chopt_out.open("simulation/" + string(catalog) + string(granularity) + "custom/" + result_files[i], ios::out | ios::trunc);

					out << "filename " << result_files[i] 
						<< " length " << file.size()
						<< " unique num " << unique_num
						<< endl;

					out << "windows ";
					for(int j=0; j<windows.size(); j++) out << windows[j] << " ";
					out << endl;

					out << "cache size ";
					for(int j=0; j<cache_size_list.size(); j++ ) out << cache_size_list[j] << " ";
					out << endl;

					out << "chopt cost ";
					for(int j=0; j<cache_size_list.size(); j++ ) out << opt_cost_list[j] << " ";
					out << endl;

					//behavior_out << "chopt result ";
					for(int k=0; k<cache_size_list.size(); k++) {
						auto cache_size = cache_size_list[k];
						if(!chopt_result[cache_size].size()) continue;
						behavior_out << cache_size << " ";
						for(int j=0; j<chopt_result[cache_size].size(); j++) behavior_out << chopt_result[cache_size][j] << " ";
						behavior_out << endl;
					}
					

					out << "belady cost ";
					for(int j=0; j<cache_size_list.size(); j++ ) out << belady_cost_list[j] << " ";
					out << endl;

					//behavior_out << "belady result ";
					for(int k=0; k<cache_size_list.size(); k++) {
						auto cache_size = cache_size_list[k];
						if(!belady_result[cache_size].size()) continue;
						behavior_out << cache_size << " ";
						for(int j=0; j<belady_result[cache_size].size(); j++) behavior_out << belady_result[cache_size][j] << " ";
						behavior_out << endl;						
					}

					out << "beladyac cost ";
					for(int j=0; j<cache_size_list.size(); j++ ) out << beladyac_cost_list[j] << " ";
					out << endl;

					//behavior_out << "belady result ";
					for(int k=0; k<cache_size_list.size(); k++) {
						auto cache_size = cache_size_list[k];
						if(!beladyac_result[cache_size].size()) continue;
						behavior_out << cache_size << " ";
						for(int j=0; j<beladyac_result[cache_size].size(); j++) behavior_out << beladyac_result[cache_size][j] << " ";
						behavior_out << endl;						
					}


					// out << "static cost ";
					// for(int j=0; j<cache_size_list.size(); j++ ) out << static_cost_list[j] << " ";
					// out << endl;

					// //behavior_out << "static result ";
					// for(int k=0; k<cache_size_list.size(); k++) {
					// 	auto cache_size = cache_size_list[k];
					// 	if(!static_result[cache_size].size()) continue;
					// 	behavior_out << cache_size << " ";
					// 	for(int j=0; j<static_result[cache_size].size(); j++) behavior_out << static_result[cache_size][j] << " ";
					// 	behavior_out << endl;	
					// }
				}
				if(isOutput && isCustom)
				{
					ofstream chopt_out;
					if(access( ("simulation/" + string(catalog)).c_str(), F_OK) != 0) 
						mkdir(("simulation/" + string(catalog)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
					if(access( ("simulation/" + string(catalog)+ string(granularity)).c_str(), F_OK) != 0) 
						mkdir(("simulation/" + string(catalog)+ string(granularity)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

					if(access( ("simulation/" + string(catalog)+ string(granularity) + "custom/").c_str(), F_OK) != 0) 
						mkdir(("simulation/" + string(catalog)+ string(granularity) + "custom/").c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

					chopt_out.open("simulation/" + string(catalog) + string(granularity) + "custom/" + result_files[i], ios::out | ios::trunc);
					for(int k=0; k<2; k++) {
						string lr_type = lr_types[k];		
						

						chopt_out << "spatial " + lr_type + " cost ";
						for(int j=0; j<cache_size_list.size(); j++ ) chopt_out << lr_spatial_cost_list[j] << " ";
						chopt_out << endl;

						chopt_out << "temporal " + lr_type + " cost ";
						for(int j=0; j<cache_size_list.size(); j++ ) chopt_out << lr_temporal_cost_list[j] << " ";
						chopt_out << endl;

						
					}
					chopt_out.close();
					

				}

				opt_cost_list.clear();
				belady_cost_list.clear();
				beladyac_cost_list.clear();
				static_cost_list.clear();
				lr_spatial_cost_list.clear();
				lr_temporal_cost_list.clear();
			
				chopt_result.clear();
				belady_result.clear();
				beladyac_result.clear();
				static_result.clear();
				lr_spatial_result.clear();
				lr_temporal_result.clear();
			}
			// lru, lfu, tinylfu, W-tinylfu, tinylfu_only(admission control), slru
			else {
						
				vector<int> lru_cost_list;
				vector<int> lfu_cost_list;
				vector<int> tinylfu_cost_list;
				vector<int> wtinylfu_cost_list;
				vector<int> tinylfu_only_cost_list;
				vector<int> slru_cost_list;

				for(int j=0; j<cache_size_list.size(); j++) {
					lru_cost_list.push_back(0);
					lfu_cost_list.push_back(0);
					tinylfu_cost_list.push_back(0);
					wtinylfu_cost_list.push_back(0);
					tinylfu_only_cost_list.push_back(0);
					slru_cost_list.push_back(0);
				}

				// cache result
				map<int, vector<int>> lru_result;
				map<int, vector<int>> lfu_result;
				map<int, vector<int>> tinylfu_result;
				map<int, vector<int>> wtinylfu_result;
				map<int, vector<int>> tinylfu_only_result;
				map<int, vector<int>> slru_result;

				omp_set_num_threads(6*cache_size_list.size());
				#pragma omp parallel for
				for(int k=0; k<6*cache_size_list.size(); k++) {
					auto j = k / 6;
					auto cache_option = k % 6;
					
					int cache_size = cache_size_list[j];
					//if(cache_size != 6) continue;
					int cost = 0;

					vector<int> res;
					vector<int> lru_res;
					vector<int> lfu_res;
					vector<int> tinylfu_res;
					vector<int> wtinylfu_res;
					vector<int> tinylfu_only_res;
					vector<int> slru_res;
					switch(cache_option) {
						
						case 0:
							
							cost = lru_monitor(file, option_list, cache_size, lru_res, isSameRW);
							lru_cost_list[j] = cost;
							lru_result[cache_size] = lru_res;
							break;
						case 1:
							
							cost = lfu_monitor(file, option_list, cache_size, lfu_res, isSameRW);
							lfu_cost_list[j] = cost;
							lfu_result[cache_size] = lfu_res;
							break;
						case 2:
							
							cost = tinylfu_monitor(file, option_list, cache_size, tinylfu_res, isSameRW);
							tinylfu_cost_list[j] = cost;
							tinylfu_result[cache_size] = tinylfu_res;
							break;
						case 3:
							
							cost = wtinylfu_monitor(file, option_list, cache_size, wtinylfu_res, isSameRW);
							wtinylfu_cost_list[j] = cost;
							wtinylfu_result[cache_size] = wtinylfu_res;
							break;
						case 4:
							
							cost = tinylfu_only_monitor(file, option_list, cache_size, tinylfu_only_res, isSameRW);
							tinylfu_only_cost_list[j] = cost;
							tinylfu_only_result[cache_size] = tinylfu_only_res;
							break;
						case 5:
							
							cost = slru_monitor(file, option_list, cache_size, slru_res, isSameRW);
							slru_cost_list[j] = cost;
							slru_result[cache_size] = slru_res;
							break;
					}
					

					#pragma omp critical
					{
						if(isDebug) cout << cache_size << " " << cache_option << " done" << endl;
					}
				}

				#pragma omp critical
				{
					cout << result_files[i] << endl;
					for(int j=0; j<cache_size_list.size(); j++) {
						cout << cache_size_list[j] 
							<< " " << lru_cost_list[j] 
							<< " " << lfu_cost_list[j] 
							<< " " << tinylfu_cost_list[j] 
							<< " " << wtinylfu_cost_list[j]
							<< " " << tinylfu_only_cost_list[j]  
							<< " " << slru_cost_list[j] 
							<< endl; 
					}
				}

				if(isOutput) {
					out.open("simulation/" + string(catalog) + string(granularity) + anal_type + result_files[i], ios::out | ios::trunc);
					behavior_out.open("cache_behavior/" + string(catalog) + string(granularity) + anal_type + result_files[i], ios::out | ios::trunc);
			
					out << "filename " << result_files[i] 
						<< " length " << file.size()
						<< " unique num " << unique_num
						<< endl;

					out << "windows ";
					for(int j=0; j<windows.size(); j++) out << windows[j] << " ";
					out << endl;

					out << "cache size ";
					for(int j=0; j<cache_size_list.size(); j++ ) out << cache_size_list[j] << " ";
					out << endl;

					out << "lru cost ";
					for(int j=0; j<cache_size_list.size(); j++ ) out << lru_cost_list[j] << " ";
					out << endl;

					//behavior_out << "lru result ";
					for(int k=0; k<cache_size_list.size(); k++) {
						auto cache_size = cache_size_list[k];
						if(!lru_result[cache_size].size()) continue;
						behavior_out << cache_size << " ";
						for(int j=0; j<lru_result[cache_size].size(); j++) behavior_out << lru_result[cache_size][j] << " ";
						behavior_out << endl;	
					}
					

					out << "lfu cost ";
					for(int j=0; j<cache_size_list.size(); j++ ) out << lfu_cost_list[j] << " ";
					out << endl;

					//behavior_out << "lfu result ";
					for(int k=0; k<cache_size_list.size(); k++) {
						auto cache_size = cache_size_list[k];
						if(!lfu_result[cache_size].size()) continue;
						behavior_out << cache_size << " ";
						for(int j=0; j<lfu_result[cache_size].size(); j++) behavior_out << lfu_result[cache_size][j] << " ";
						behavior_out << endl;	
					}
					

					out << "tinylfu cost ";
					for(int j=0; j<cache_size_list.size(); j++ ) out << tinylfu_cost_list[j] << " ";
					out << endl;

					//behavior_out << "tinylfu result ";
					for(int k=0; k<cache_size_list.size(); k++) {
						auto cache_size = cache_size_list[k];
						if(!tinylfu_result[cache_size].size()) continue;
						behavior_out << cache_size << " ";
						for(int j=0; j<tinylfu_result[cache_size].size(); j++) behavior_out << tinylfu_result[cache_size][j] << " ";
						behavior_out << endl;	
					}
					

					out << "wtinylfu cost ";
					for(int j=0; j<cache_size_list.size(); j++ ) out << wtinylfu_cost_list[j] << " ";
					out << endl;

					//behavior_out << "wtinylfu result ";
					for(int k=0; k<cache_size_list.size(); k++) {
						auto cache_size = cache_size_list[k];
						if(!wtinylfu_result[cache_size].size()) continue;
						behavior_out << cache_size << " ";
						for(int j=0; j<wtinylfu_result[cache_size].size(); j++) behavior_out << wtinylfu_result[cache_size][j] << " ";
						behavior_out << endl;	
					}
					

					out << "tinylfu only cost ";
					for(int j=0; j<cache_size_list.size(); j++ ) out << tinylfu_only_cost_list[j] << " ";
					out << endl;

					//behavior_out << "tinylfu only result ";
					for(int k=0; k<cache_size_list.size(); k++) {
						auto cache_size = cache_size_list[k];
						if(!tinylfu_only_result[cache_size].size()) continue;
						behavior_out << cache_size << " ";
						for(int j=0; j<tinylfu_only_result[cache_size].size(); j++) behavior_out << tinylfu_only_result[cache_size][j] << " ";
						behavior_out << endl;	
					}
					

					out << "slru cost ";
					for(int j=0; j<cache_size_list.size(); j++ ) out << slru_cost_list[j] << " ";
					out << endl;

					//behavior_out << "slru result ";
					for(int k=0; k<cache_size_list.size(); k++) {
						auto cache_size = cache_size_list[k];
						if(!slru_result[cache_size].size()) continue;
						behavior_out << cache_size << " ";
						for(int j=0; j<slru_result[cache_size].size(); j++) behavior_out << slru_result[cache_size][j] << " ";
						behavior_out << endl;	
					}
					
				}

				lru_cost_list.clear();
				lfu_cost_list.clear();
				tinylfu_cost_list.clear();
				tinylfu_only_cost_list.clear();
				wtinylfu_cost_list.clear();
				slru_cost_list.clear();

				lru_result.clear();
				lfu_result.clear();
				tinylfu_result.clear();
				wtinylfu_result.clear();
				tinylfu_only_result.clear();
				slru_result.clear();
			}

			// Output matrix file: for each source file
			// filename, length, unique num, 
			// "windows", windows, 
			// "cache size", cache size, 
			// "opt cost", opt_cost[cache_size], 
			// "lru cost", lru_cost[cache_size],
			// "results",
			// item, option, isScan, rd, age, 
			// frequency[windows],
			// result[cache_size]
			
	/*
			if(isOutput) {
				out << "results" << endl;
				for(int j=0; j<file.size(); j++) {
					out << file[j] << " "
						<< option_list[j] << " "
						<< scans[j] << " "
						<< rd[j] << " "
						<< age[j] << endl;
					for(int k=0; k<windows.size(); k++) out << frequency[j][k] << " ";
					out << endl;
					for(int k=0; k<cache_size_list.size(); k++) out << result[cache_size_list[k]][j] << " ";
					out << endl;
				}
				out << endl;
			}
	*/	
			file.clear();
			option_list.clear();
			cache_size_list.clear();
			windows.clear();

			if (isOutput)  {
				out.close();
				behavior_out.close();
			}

			#pragma omp critical 
			{
				cout << result_files[i] << " done" << endl;
			}
		}
	}
	
	

	result_files.clear();
	return;
}

void handle_sampling(const char* result_dir, 
	const char* log_dir, 
	const char* rd_dir,
	const char* source_dir, 
	const char* catalog,
	const char* granularity,
	bool isOutput = false,
	bool isDebug = false) {

	string anal_type = "offline/";

	if(isOutput) {
		if(access( ("simulation/" + string(catalog)).c_str(), F_OK) != 0) 
			mkdir(("simulation/" + string(catalog)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(access( ("simulation/" + string(catalog)+ string(granularity)).c_str(), F_OK) != 0) 
			mkdir(("simulation/" + string(catalog)+ string(granularity)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(access( ("simulation/" + string(catalog)+ string(granularity) + anal_type).c_str(), F_OK) != 0) 
			mkdir(("simulation/" + string(catalog)+ string(granularity) + anal_type).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(access( ("cache_behavior/" + string(catalog)).c_str(), F_OK) != 0) 
			mkdir(("cache_behavior/" + string(catalog)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(access( ("cache_behavior/" + string(catalog)+ string(granularity)).c_str(), F_OK) != 0) 
			mkdir(("cache_behavior/" + string(catalog)+ string(granularity)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(access( ("cache_behavior/" + string(catalog)+ string(granularity) + anal_type).c_str(), F_OK) != 0) 
			mkdir(("cache_behavior/" + string(catalog)+ string(granularity) + anal_type).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);		
		if(access( ("chopt/" + string(catalog)).c_str(), F_OK) != 0) 
			mkdir(("chopt/" + string(catalog)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(access( ("chopt/" + string(catalog)+ string(granularity)).c_str(), F_OK) != 0) 
			mkdir(("chopt/" + string(catalog)+ string(granularity)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);		
	}

	string target_file = string(granularity).substr(0, string(granularity).size()-1);
	if(isDebug) cout << "target file is " << target_file << endl;

	vector<string> result_files;

	Load _load;
	_load.ReadDirectory(dir_as_char(log_dir, catalog, granularity),result_files);
 
	//for(auto it = result_files.begin(); it != result_files.end(); ++it) cout << *it << endl;


	// vector<string> _temp;
	// for(auto it = result_files.begin(); it != result_files.end(); ++it) {
	// 	auto fname = *it;
	// 	size_t found = fname.find_first_of("-");
	// 	size_t found2 = fname.find_last_of("-");
	// 	string sampling_rate = fname.substr(found+1, found2-found-1);
	// 	string sampling_order = fname.substr(found2+1, -1);
		
	// 	if(sampling_rate == "5") _temp.push_back(*it);
	// 	//if(sampling_rate == "20" && (sampling_order == "6" || sampling_order == "7" || sampling_order == "8" || sampling_order == "9")) _temp.push_back(*it);
	// }
	// result_files.clear();
	// for(auto it = _temp.begin(); it != _temp.end(); ++it) result_files.push_back(*it);
 	
	sort(result_files.begin(), result_files.end());

	vector<int> cache_size_list_all;
	vector<int> windows_all;

	load_sampling_cache(dir_as_string(log_dir, catalog, granularity), target_file+"-20-0", cache_size_list_all, windows_all);

	// for root file, need to calculate size*100, size*20, size*10, size*5
	auto it = find(result_files.begin(), result_files.end(), target_file);
	if (it != result_files.end()) result_files.erase(it);
	bool isRoot = false;
	bool isNormal = true;
	if(isRoot) {
		
		Load load;
		// original file
		vector<string> file;
		// read/write
		vector<bool> option_list;
		if(isDebug) cout << "root file loading begin" << endl;	
		load.LoadFile((dir_as_string(source_dir, catalog) + target_file + ".csv").c_str(), file, option_list);
		if(isDebug) cout << "root file loading end" << endl;
		vector<int> next_access_list;
		calc_next_access_list(file, next_access_list);
		if(isDebug) cout << target_file << " " << file.size() << " " << next_access_list.size() << endl;


		int cache_size_rate[4] = {100, 20, 10, 5};
		
		map<int, int> _cache_size_list;
		for(int i=0; i<4; i++) {
			for(auto it = cache_size_list_all.begin(); it != cache_size_list_all.end(); ++it){
				//cache_size_list.push_back((*it) * cache_size_rate[i]);
				_cache_size_list[(*it) * cache_size_rate[i]] = 1;
			}
		}
		vector<int> _cache_sizes;
		for(auto it = _cache_size_list.begin(); it != _cache_size_list.end(); ++it) _cache_sizes.push_back(it->first);
		_cache_size_list.clear();

		// opt cache swap in/out result
		map<int, vector<int>> _swap_in;
		map<int, vector<int>> _swap_out;

		// for all cache sizes, calculate swap in/out
		if(isDebug) cout << "log file beginning" << endl;
		//log(dir_as_string(log_dir, catalog, granularity), result_files[i], cache_size_list, swap_in, swap_out, 2*file.size()+2);
		int unique_num = log_new(dir_as_string(log_dir, catalog, granularity), target_file, _cache_sizes, _swap_in, _swap_out);
		if(isDebug) {
			cout << "log file loaded" << " " << _swap_in.size() << " " << _swap_out.size() << endl;
		}

		for(int i=0; i<4; i++) {
			cout << "root " << target_file << " " << cache_size_rate[i] << endl;
			// output file
			ofstream out;
			ofstream behavior_out;

			vector<int> cache_size_list;
			vector<int> windows;
			
			for(auto it = cache_size_list_all.begin(); it != cache_size_list_all.end(); ++it){
				if((*it) * cache_size_rate[i] < unique_num ) cache_size_list.push_back((*it) * cache_size_rate[i]);
			} 
			for(auto it = windows_all.begin(); it != windows_all.end(); ++it) windows.push_back(*it);

			if(isDebug) {
				cout << "cache size" << endl;
				for(auto it = cache_size_list.begin(); it !=cache_size_list.end(); ++it) cout << *it << " ";
				cout << endl;
				cout << "windows size" << endl;
				for(auto it = windows.begin(); it !=windows.end(); ++it) cout << *it << " ";
				cout << endl;
			}

			// opt cache swap in/out result
			map<int, vector<int>> swap_in;
			map<int, vector<int>> swap_out;

			for(auto it = cache_size_list.begin(); it != cache_size_list.end(); ++it) {
				swap_in[*it] = _swap_in[*it];
				swap_out[*it] = _swap_out[*it];	 
			}
			
			if(isDebug) {
				cout << "swap in out loaded" << " " << swap_in.size() << " " << swap_out.size() << endl;
				for (auto it = swap_in.begin(); it != swap_in.end(); ++it) cout << it->second.size() << " ";
				cout << endl;
				for (auto it = swap_out.begin(); it != swap_out.end(); ++it) cout << it->second.size() << " ";
				cout << endl;
			}

			// cache result
			map<int, vector<int>> chopt_result;
			map<int, vector<int>> belady_result;
			// map<int, vector<int>> wtinylfu_result;

			// opt and lru cache cost		
			vector<int> opt_cost_list;
			vector<int> belady_cost_list;
			// vector<int> wtinylfu_cost_list;

			for(int j=0; j<cache_size_list.size(); j++) {
				opt_cost_list.push_back(0);
				belady_cost_list.push_back(0);
				// wtinylfu_cost_list.push_back(0);
			}

			omp_set_num_threads(2*cache_size_list.size());
			#pragma omp parallel for
			for(int k=0; k<2*cache_size_list.size(); k++) {
				auto j = k / 2;
				auto cache_option = k % 2;

				int cache_size = cache_size_list[j];
				int cost=0;
				
				if(swap_in[cache_size].size() == 0 || swap_out[cache_size].size() == 0) continue; 
					vector<int> chopt_res;
					vector<int> belady_res;
					vector<int> wtinylfu_res;
					
					switch(cache_option) {
						case 0:
							cost = chopt_monitor(file, option_list, swap_in[cache_size], swap_out[cache_size], chopt_res);
							opt_cost_list[j] = cost;
							chopt_result[cache_size] = chopt_res;
							break;
						case 1:	
							#pragma omp critical
							{
								cout << "belady " << cache_size << " begin\n";
							}
							cost = belady_monitor(file, option_list, next_access_list, cache_size, belady_res);
							belady_cost_list[j] = cost;
							belady_result[cache_size] = belady_res;
							#pragma omp critical
							{
								cout << "belady " << cache_size << " end\n";
							}
							break;
						// case 2:	
						// 	cost = wtinylfu_monitor(file, option_list, cache_size, wtinylfu_res);
						// 	wtinylfu_cost_list[j] = cost;
						// 	wtinylfu_result[cache_size] = wtinylfu_res;
						// 	break;
					}


				#pragma omp critical 
				{
					if(isDebug) cout << cache_size << " " << cache_option << " done" << endl;
				}
			}

			#pragma omp barrier
			
			
			cout << target_file << endl;
			for(int j=0; j<cache_size_list.size(); j++) {
				cout << cache_size_list[j] << " " << opt_cost_list[j] << " " << belady_cost_list[j] << endl; 
			}
			

			if(isOutput) {
				out.open("simulation/" + string(catalog) + string(granularity) + anal_type + target_file + "_" + to_string(cache_size_rate[i]), ios::out | ios::trunc);
				behavior_out.open("cache_behavior/" + string(catalog) + string(granularity) + anal_type + target_file + "_" + to_string(cache_size_rate[i]), ios::out | ios::trunc);
				//ofstream chopt_out;
				//chopt_out.open("simulation/" + string(catalog) + string(granularity) + "custom/" + result_files[i], ios::out | ios::trunc);

				out << "filename " << target_file 
					<< " length " << file.size()
					<< " unique num " << unique_num
					<< endl;

				out << "windows ";
				for(int j=0; j<windows.size(); j++) out << windows[j] << " ";
				out << endl;

				out << "cache size ";
				for(int j=0; j<cache_size_list.size(); j++ ) out << cache_size_list[j] << " ";
				out << endl;

				out << "chopt cost ";
				for(int j=0; j<cache_size_list.size(); j++ ) out << opt_cost_list[j] << " ";
				out << endl;

				//behavior_out << "chopt result ";
				for(int k=0; k<cache_size_list.size(); k++) {
					auto cache_size = cache_size_list[k];
					if(!chopt_result[cache_size].size()) continue;
					behavior_out << cache_size << " ";
					for(int j=0; j<chopt_result[cache_size].size(); j++) behavior_out << chopt_result[cache_size][j] << " ";
					behavior_out << endl;
				}
				

				out << "belady cost ";
				for(int j=0; j<cache_size_list.size(); j++ ) out << belady_cost_list[j] << " ";
				out << endl;

				//behavior_out << "belady result ";
				for(int k=0; k<cache_size_list.size(); k++) {
					auto cache_size = cache_size_list[k];
					if(!belady_result[cache_size].size()) continue;
					behavior_out << cache_size << " ";
					for(int j=0; j<belady_result[cache_size].size(); j++) behavior_out << belady_result[cache_size][j] << " ";
					behavior_out << endl;						
				}

				out.close();
				behavior_out.close();
			}

			if(isDebug) {
				cout << "clearing" << endl;
				cout << "swapping in and out " << swap_in.size() << " " << swap_out.size() << endl;
				for (auto it = swap_in.begin(); it != swap_in.end(); ++it) cout << it->second.size() << " ";
					cout << endl;
				for (auto it = swap_out.begin(); it != swap_out.end(); ++it) cout << it->second.size() << " ";
					cout << endl;
			}
			for (auto it = swap_in.begin(); it != swap_in.end(); ++it) it->second.clear();
			for (auto it = swap_out.begin(); it != swap_out.end(); ++it) it->second.clear();
			swap_in.clear();
			swap_out.clear();
			
			if(isDebug) {
				cout << "cache_size_list " << cache_size_list.size() << endl;
			}
			cache_size_list.clear();
			if(isDebug) {
				cout << "windows " << windows.size() << endl;
			}
			windows.clear();
			
			if(isDebug) {
				cout << "results clear" << endl;
			}

			opt_cost_list.clear();
			belady_cost_list.clear();
			// wtinylfu_cost_list.clear();

			chopt_result.clear();
			belady_result.clear();
			// wtinylfu_result.clear();

			cout << target_file + "_" + to_string(cache_size_rate[i]) << " done" << endl;
		}
		if(isDebug) {
			cout << "file " << file.size() << endl;
		}
		file.clear();
		if(isDebug) {
			cout << "option list " << option_list.size() << endl;
		}
		option_list.clear();
		if(isDebug) {
			cout << "next access list " << next_access_list.size() << endl;
		}
		next_access_list.clear();
	}

	if (!isNormal) return;

	for(int i=0; i< result_files.size(); i++) {
		// output file
		ofstream out;
		ofstream behavior_out;

		Load load;
		// original file
		vector<string> file;
		// read/write
		vector<bool> option_list;
		
		load.LoadFile((dir_as_string(source_dir, catalog) + result_files[i] + ".csv").c_str(), file, option_list);

		vector<int> next_access_list;
		calc_next_access_list(file, next_access_list);
		if(isDebug) cout << result_files[i] << " " << file.size() << " " << next_access_list.size() << endl;

		// opt cache swap in/out result
		map<int, vector<int>> swap_in;
		map<int, vector<int>> swap_out;

		// for all cache sizes, calculate swap in/out
		if(isDebug) cout << "log file beginning" << endl;
		//log(dir_as_string(log_dir, catalog, granularity), result_files[i], cache_size_list, swap_in, swap_out, 2*file.size()+2);
		int unique_num = log_new(dir_as_string(log_dir, catalog, granularity), result_files[i], cache_size_list_all, swap_in, swap_out);
		if(isDebug) {
			cout << "log file loaded" << " " << swap_in.size() << " " << swap_out.size() << endl;
			for (auto it = swap_in.begin(); it != swap_in.end(); ++it) cout << it->second.size() << " ";
			cout << endl;
			for (auto it = swap_out.begin(); it != swap_out.end(); ++it) cout << it->second.size() << " ";
			cout << endl;
		} 
		
		vector<int> cache_size_list;
		vector<int> windows;
		
		for(auto it = cache_size_list_all.begin(); it != cache_size_list_all.end(); ++it){
			if (*it > unique_num) break;
			cache_size_list.push_back(*it);
		} 
		for(auto it = windows_all.begin(); it != windows_all.end(); ++it) windows.push_back(*it);

		if(isDebug) {
			cout << "cache size" << endl;
			for(auto it = cache_size_list.begin(); it !=cache_size_list.end(); ++it) cout << *it << " ";
			cout << endl;
			cout << "windows size" << endl;
			for(auto it = windows.begin(); it !=windows.end(); ++it) cout << *it << " ";
			cout << endl;
		}

		// cache result
		map<int, vector<int>> chopt_result;
		map<int, vector<int>> belady_result;
		// map<int, vector<int>> wtinylfu_result;

		// opt and lru cache cost		
		vector<int> opt_cost_list;
		vector<int> belady_cost_list;
		// vector<int> wtinylfu_cost_list;

		for(int j=0; j<cache_size_list.size(); j++) {
			opt_cost_list.push_back(0);
			belady_cost_list.push_back(0);
			// wtinylfu_cost_list.push_back(0);
		}

		omp_set_num_threads(2*cache_size_list.size());
		#pragma omp parallel for
		for(int k=0; k<2*cache_size_list.size(); k++) {
			auto j = k / 2;
			auto cache_option = k % 2;

			int cache_size = cache_size_list[j];
			int cost=0;
			
			if(swap_in[cache_size].size() == 0 || swap_out[cache_size].size() == 0) continue; 
				vector<int> chopt_res;
				vector<int> belady_res;
				vector<int> wtinylfu_res;
				
				switch(cache_option) {
					case 0:
						cost = chopt_monitor(file, option_list, swap_in[cache_size], swap_out[cache_size], chopt_res);
						opt_cost_list[j] = cost;
						chopt_result[cache_size] = chopt_res;
						break;
					case 1:	
						#pragma omp critical
						{
							cout << "belady " << cache_size << " begin\n";
						}
						cost = belady_monitor(file, option_list, next_access_list, cache_size, belady_res);
						belady_cost_list[j] = cost;
						belady_result[cache_size] = belady_res;
						#pragma omp critical
						{
							cout << "belady " << cache_size << " end\n";
						}
						break;
					// case 2:	
					// 	cost = wtinylfu_monitor(file, option_list, cache_size, wtinylfu_res);
					// 	wtinylfu_cost_list[j] = cost;
					// 	wtinylfu_result[cache_size] = wtinylfu_res;
					// 	break;
				}


			#pragma omp critical 
			{
				if(isDebug) cout << cache_size << " " << cache_option << " done" << endl;
			}
		}

		#pragma omp barrier
		
		
		cout << result_files[i] << endl;
		for(int j=0; j<cache_size_list.size(); j++) {
			cout << cache_size_list[j] << " " << opt_cost_list[j] << " " << belady_cost_list[j] << endl; 
		}
		

		if(isOutput) {
			out.open("simulation/" + string(catalog) + string(granularity) + anal_type + result_files[i], ios::out | ios::trunc);
			behavior_out.open("cache_behavior/" + string(catalog) + string(granularity) + anal_type + result_files[i], ios::out | ios::trunc);
			//ofstream chopt_out;
			//chopt_out.open("simulation/" + string(catalog) + string(granularity) + "custom/" + result_files[i], ios::out | ios::trunc);

			out << "filename " << result_files[i] 
				<< " length " << file.size()
				<< " unique num " << unique_num
				<< endl;

			out << "windows ";
			for(int j=0; j<windows.size(); j++) out << windows[j] << " ";
			out << endl;

			out << "cache size ";
			for(int j=0; j<cache_size_list.size(); j++ ) out << cache_size_list[j] << " ";
			out << endl;

			out << "chopt cost ";
			for(int j=0; j<cache_size_list.size(); j++ ) out << opt_cost_list[j] << " ";
			out << endl;

			//behavior_out << "chopt result ";
			for(int k=0; k<cache_size_list.size(); k++) {
				auto cache_size = cache_size_list[k];
				if(!chopt_result[cache_size].size()) continue;
				behavior_out << cache_size << " ";
				for(int j=0; j<chopt_result[cache_size].size(); j++) behavior_out << chopt_result[cache_size][j] << " ";
				behavior_out << endl;
			}
			

			out << "belady cost ";
			for(int j=0; j<cache_size_list.size(); j++ ) out << belady_cost_list[j] << " ";
			out << endl;

			//behavior_out << "belady result ";
			for(int k=0; k<cache_size_list.size(); k++) {
				auto cache_size = cache_size_list[k];
				if(!belady_result[cache_size].size()) continue;
				behavior_out << cache_size << " ";
				for(int j=0; j<belady_result[cache_size].size(); j++) behavior_out << belady_result[cache_size][j] << " ";
				behavior_out << endl;						
			}

			out.close();
			behavior_out.close();
		}

		if(isDebug) {
			cout << "clearing" << endl;
			cout << "swapping in and out " << swap_in.size() << " " << swap_out.size() << endl;
			for (auto it = swap_in.begin(); it != swap_in.end(); ++it) cout << it->second.size() << " ";
				cout << endl;
			for (auto it = swap_out.begin(); it != swap_out.end(); ++it) cout << it->second.size() << " ";
				cout << endl;
		}
		for (auto it = swap_in.begin(); it != swap_in.end(); ++it) it->second.clear();
		for (auto it = swap_out.begin(); it != swap_out.end(); ++it) it->second.clear();
		swap_in.clear();
		swap_out.clear();
		if(isDebug) {
			cout << "file " << file.size() << endl;
		}
		file.clear();
		if(isDebug) {
			cout << "option list " << option_list.size() << endl;
		}
		option_list.clear();
		if(isDebug) {
			cout << "cache_size_list " << cache_size_list.size() << endl;
		}
		cache_size_list.clear();
		if(isDebug) {
			cout << "windows " << windows.size() << endl;
		}
		windows.clear();
		
		if(isDebug) {
			cout << "results clear" << endl;
		}

		opt_cost_list.clear();
		belady_cost_list.clear();
		// wtinylfu_cost_list.clear();

		chopt_result.clear();
		belady_result.clear();
		// wtinylfu_result.clear();

		cout << result_files[i] << " done" << endl;
	}
	
	
	cache_size_list_all.clear();
	windows_all.clear();
	result_files.clear();
	return;
}


void analyze_wtinylfu(const char* result_dir, 
	const char* log_dir,
	const char* source_dir, 
	const char* catalog,
	const char* granularity, 
	bool isOutput = false,
	bool isDebug = false) {

	if(isOutput) {				
		if(access( ("tinylfu/"+ string(catalog)).c_str(), F_OK) != 0)  
			mkdir(("tinylfu/"+ string(catalog)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(access( ("tinylfu/"+ string(catalog)+ string(granularity)).c_str(), F_OK) != 0)  
			mkdir(("tinylfu/"+ string(catalog)+ string(granularity)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	}

	vector<string> result_files;
	Load _load;
	_load.ReadDirectory(dir_as_char(result_dir, catalog, granularity),result_files);
	if(catalog == "short/") {
		for(auto it = result_files.begin(); it != result_files.end(); ++it) {
			if ( (*it) == "trace_canneal" ) result_files.erase(it);
			if ( (*it) == "1000" ) result_files.erase(it);
			if ( (*it) == "max" ) result_files.erase(it);
		}
	}
    sort(result_files.begin(), result_files.end());
	//result_files.push_back("trace_radiosity");
	//result_files.push_back("trace_streamcluster");

	int range[6] = {0,20,40,60,80,106};
	int end_loop = 5;

	if(catalog == "short/") {
		range[1] = result_files.size();
		end_loop = 1;
	}

	for (int l=0; l<end_loop; l++) {
		int round_start = range[l];
		int round_end = range[l+1];
		//int round_start = 0;
		//int round_end = result_files.size();
		omp_set_num_threads(round_end - round_start);

		//for (int i=0; i<result_files.size(); i++) {
		#pragma omp parallel for
		for(int i=round_start; i< round_end; i++) {
			if(isOutput) {				
				if(access( ("tinylfu/"+ string(catalog)+ string(granularity) + result_files[i]).c_str(), F_OK) != 0)  
					mkdir(("tinylfu/"+ string(catalog)+ string(granularity) + result_files[i]).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			}

			if(isDebug) cout << "file " << result_files[i] << endl;

			Load load;
			// original file
			vector<string> file;
			// read/write
			vector<bool> option_list;

			if(catalog == "cp/") load.LoadCPFile((dir_as_string(source_dir, catalog) + result_files[i] + ".vscsitrace.sample").c_str(), file, option_list);
			else load.LoadFile((dir_as_string(source_dir, catalog) + result_files[i] + ".csv").c_str(), file, option_list);
			int unique_num = load.UniqueNum(file);
			if(isDebug) cout <<unique_num << endl;

			int cache_size_temp = 0;
			vector<int> cache_size_list;
			vector<int> windows;

			load_cache_and_window_list(dir_as_string(log_dir, catalog, granularity), result_files[i], file.size(), cache_size_list, windows);


			//#pragma omp parallel for
			for(int j=0; j<cache_size_list.size(); j++) {			
				vector<int> window_cost_list;
				vector<int> bf_cost_list;
				vector<int> cbf_cost_list;
				vector<int> slru_cost_list;
				vector<int> w_cost_list;
				
				int cache_size = cache_size_list[j];
				int cost = 0;

				ofstream out;
				// output
				if(isOutput)
				out.open("tinylfu/" + string(catalog) + string(granularity) + result_files[i] + "/" + to_string(cache_size), ios::out | ios::trunc);

				vector<int> res;

				// tinylfu baseline receipe: 
				// 		1% window size
				// 		1024 bf&cbf size
				// 		80% protect and 20% prob for SLRU
				// 		1000 W
				int _window_size = cache_size / 100 == 0? 1 : cache_size/100;
				if(cache_size == 2) _window_size = 0;
				int _bf_size = 1024;
				int _cbf_size = 1024;
				int _prob_size = cache_size / 5 == 0? 1: cache_size / 5;
				int _protect_size = cache_size - _window_size - _prob_size;
				int _w = 1000;

				/*cost = wtinylfu_test_monitor(file, 
					option_list, 
					cache_size,	
					_window_size,
					_bf_size,
					_cbf_size,
					_prob_size,
					_protect_size,
					_w, wtinylfu_res);*/

				// test option:
				// 1. window size
				vector<int> window_size_test_list;
				int base = cache_size / 100;
				if(base == 0) base = 1;
				for(int k=1; k<=10; k++) {
					window_size_test_list.push_back(base*k);
					cost = wtinylfu_test_monitor(file, option_list, cache_size,	base*k, _bf_size, _cbf_size, _prob_size, _protect_size, _w, res);
					window_cost_list.push_back(cost);
				}

				// 2. bf size
				// 3. cbf size
				// 5. w size
				vector<int> counter_test_list;
				base = 16;
				while(base <= 4096) {
					counter_test_list.push_back(base);
					cost = wtinylfu_test_monitor(file, option_list, cache_size,	_window_size, base, _cbf_size, _prob_size, _protect_size, _w, res);
					bf_cost_list.push_back(cost);
					cost = wtinylfu_test_monitor(file, option_list, cache_size,	_window_size, _bf_size, base, _prob_size, _protect_size, _w, res);
					cbf_cost_list.push_back(cost);
					cost = wtinylfu_test_monitor(file, option_list, cache_size,	_window_size, _bf_size, _cbf_size, _prob_size, _protect_size, base, res);
					w_cost_list.push_back(cost);
					base *= 2;
				}

				// 4. slru rate
				vector<int> prob_test_list;
				vector<int> protect_test_list;
				base = cache_size / 10;
				if(base == 0) base = 1;
				for(int k=1; k<=10; k++) {
					int temp_prob = k * base;
					int temp_protect = cache_size - _window_size - temp_prob;
					if(temp_protect < 1) break;
					prob_test_list.push_back(temp_prob);
					protect_test_list.push_back(temp_protect);
				}
				for(int k=0; k<prob_test_list.size(); k++) {
					cost = wtinylfu_test_monitor(file, option_list, cache_size,	_window_size, _bf_size, _cbf_size, prob_test_list[k], protect_test_list[k], _w, res);
					slru_cost_list.push_back(cost);
				}
				
				#pragma omp critical 
				{
					if(isDebug) cout << result_files[i] << " " << cache_size << " done" << endl;
				}

				if(isOutput) {
					out << cache_size << endl;

					for(int k=0; k<window_size_test_list.size(); k++) out << window_size_test_list[k] << " ";
					out << endl;

					for(int k=0; k<window_cost_list.size(); k++) out << window_cost_list[k] << " ";
					out << endl;

					for(int k=0; k<counter_test_list.size(); k++) out << counter_test_list[k] << " ";
					out << endl;

					for(int k=0; k<bf_cost_list.size(); k++) out << bf_cost_list[k] << " ";
					out << endl;

					for(int k=0; k<cbf_cost_list.size(); k++) out << cbf_cost_list[k] << " ";
					out << endl;

					for(int k=0; k<w_cost_list.size(); k++) out << w_cost_list[k] << " ";
					out << endl;

					for(int k=0; k<prob_test_list.size(); k++) out << prob_test_list[k] << " ";
					out << endl;

					for(int k=0; k<protect_test_list.size(); k++) out << protect_test_list[k] << " ";
					out << endl;

					for(int k=0; k<slru_cost_list.size(); k++) out << slru_cost_list[k] << " ";
					out << endl;

					out.close();				
				}

				window_cost_list.clear();
				bf_cost_list.clear();
				cbf_cost_list.clear();
				slru_cost_list.clear();
				w_cost_list.clear();

				res.clear();
				prob_test_list.clear();
				protect_test_list.clear();
				counter_test_list.clear();
				window_size_test_list.clear();
			}

			file.clear();
			option_list.clear();
			cache_size_list.clear();
			windows.clear();

			#pragma omp critical 
			{
				cout << result_files[i] << " done" << endl;
			}
		}
	}
	
	result_files.clear();
	return;
}


void custom_cache(const char* result_dir, 
	const char* log_dir,
	const char* source_dir, 
	const char* catalog,
	const char* granularity, 
	bool isOutput = false,
	bool isDebug = false) {

	if(isOutput) {				
		if(access( ("custom/"+ string(catalog)).c_str(), F_OK) != 0)  
			mkdir(("custom/"+ string(catalog)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(access( ("custom/"+ string(catalog)+ string(granularity)).c_str(), F_OK) != 0)  
			mkdir(("custom/"+ string(catalog)+ string(granularity)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	}

	vector<string> result_files;
	Load _load;
	_load.ReadDirectory(dir_as_char(result_dir, catalog, granularity),result_files);
	if(catalog == "short/") {
		for(auto it = result_files.begin(); it != result_files.end(); ++it) {
			if ( (*it) == "trace_canneal" ) result_files.erase(it);
			if ( (*it) == "1000" ) result_files.erase(it);
			if ( (*it) == "max" ) result_files.erase(it);
		}
	}
    sort(result_files.begin(), result_files.end());
	// result_files.push_back("trace_radiosity");
	// result_files.push_back("trace_ocean_cp");

	int range[6] = {0,20,40,60,80,106};
	int end_loop = 5;

	if(catalog == "short/" || catalog == "parsec/") {
		range[1] = result_files.size();
		end_loop = 1;
	}

	for (int l=0; l<end_loop; l++) {
		int round_start = range[l];
		int round_end = range[l+1];
		//int round_start = 0;
		//int round_end = result_files.size();
		// omp_set_num_threads(round_end - round_start);

		//for (int i=0; i<result_files.size(); i++) {
		// #pragma omp parallel for
		for(int i=round_start; i< round_end; i++) {
			time_t begin, end;
			time(&begin);
			if(isOutput) {				
				if(access( ("custom/"+ string(catalog)).c_str(), F_OK) != 0)  
					mkdir(("custom/"+ string(catalog)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
				if(access( ("custom/"+ string(catalog)+ string(granularity)).c_str(), F_OK) != 0)  
					mkdir(("custom/"+ string(catalog)+ string(granularity)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
				if(access( ("custom/"+ string(catalog)+ string(granularity) + result_files[i]).c_str(), F_OK) != 0)  
					mkdir(("custom/"+ string(catalog)+ string(granularity) + result_files[i]).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			}

			if(isDebug) cout << "file " << result_files[i] << endl;

			Load load;
			// original file
			vector<string> file;
			// read/write
			vector<bool> option_list;

			if(catalog == "cp/") load.LoadCPFile((dir_as_string(source_dir, catalog) + result_files[i] + ".vscsitrace.sample").c_str(), file, option_list);
			else load.LoadFile((dir_as_string(source_dir, catalog) + result_files[i] + ".csv").c_str(), file, option_list);
			int unique_num = load.UniqueNum(file);
			if(isDebug) cout <<unique_num << endl;

			vector<int> next_access_list;
			calc_next_access_list(file, next_access_list);

			int cache_size_temp = 0;
			vector<int> cache_size_list;
			vector<int> windows;

			load_cache_and_window_list(dir_as_string(log_dir, catalog, granularity), result_files[i], file.size(), cache_size_list, windows);

			// for each cache size
			// 		(window type: true for metadata cache, false for item cache)
			// 		for different window size : (1% -> 20%?)
			// 			TinyLFU, LRU, SLRU, OPT, (half) LRU, SLRU, OPT

			// int cost_res[cache_size_list.size()][3][20];
			map<int, map<int, map<int, int>>> cost_res;

			// cache_size: same as cache_size_list
			// window size: size/16 * (0 - 9)
			// three main cache options
			// vary if discount or not
			// vary if count first or not
			// cache type:
			// 

			omp_set_num_threads(40);
			#pragma omp parallel for
			for (int j=0; j<cache_size_list.size()*10*13; j++) {
				int cache_size = cache_size_list[j/130];
				int window_size_rate = (j%130)/13;
				int cache_type = j%13;
				// 0-5: no discount
				// 6-11: discount
				// 0-2, 6-8: count before window
				// 3-5, 9-11: count after window
				// 12: wtinylfu
				// 0,3,6,9: LRU
				// 1,4,7,10: SLRU
				// 2,5,8,11: Belady

				int cost = 0;

				int window_size_base = cache_size / 16 == 0? 1 : cache_size/16;
				int window_size = window_size_base * window_size_rate;
	
				if(window_size_base * (window_size_rate-1) >= cache_size - 1) continue;
				
				int window_option = 1;
				if(window_size == 0) window_option = 0;
				if(window_size >= cache_size) window_size = cache_size - 1;			
				cache_size -= window_size;

				int w = cache_type/6 == 0? 0 : 1000;
				int counter_option = 0;
				if(cache_type != 12 && (cache_type % 6) < 3) counter_option = 1;
				
				if(cache_type == 12) {
					vector<int> res;
					int prob_size = cache_size / 5 == 0? 1: cache_size / 5;
					int protect_size = cache_size - prob_size;
					cost = wtinylfu_test_monitor(file, option_list, window_size + cache_size, window_size, 1024, 1024, prob_size, protect_size, 1000, res);
					res.clear();
				} 
				else if(cache_type % 3 == 2) {
					vector<int> res;
					cost = custom_monitor(file, option_list, cache_type % 3, window_option, counter_option, window_size, cache_size, w, res, next_access_list);
					res.clear();
				} 
				else {
					vector<int> res;
					cost = custom_monitor(file, option_list, cache_type % 3, window_option, counter_option, window_size, cache_size, w, res);
					res.clear();
				} 

				#pragma omp critical
				{
					cost_res[cache_size+window_size][cache_type][window_size] = cost;
					if(isDebug) cout << result_files[i] << " " << window_size + cache_size << " " << window_size_rate << " " << cache_type << " done, cost is " << cost << endl;
				}
			}

			// output format: for each cache size
			// 		for each cache type
			// 			window size list
			// 			cost
			if(isOutput) {
				for (auto it = cost_res.begin(); it != cost_res.end(); ++it) {
					auto cache_size = it->first;
					
					ofstream out;
					out.open("custom/" + string(catalog) + string(granularity) + result_files[i] + "/" + to_string(cache_size), ios::out | ios::trunc);
					vector<int> window_size_vec;

					for(auto iit = (it->second)[0].begin(); iit != (it->second)[0].end(); ++iit){
						out << iit->first << " ";
						window_size_vec.push_back(iit->first);
					}
					out << endl;

					for (int cache_type = 0; cache_type < 13; cache_type++) {
						for(auto iit = window_size_vec.begin(); iit != window_size_vec.end(); ++iit) {
							out << (it->second)[cache_type][*iit] << " ";
						} 
						out << endl;
					}

					window_size_vec.clear();
					out.close();
				}
			}
			time(&end);
			cout << result_files[i] << " " << " time takes " << end-begin << "s" << endl;


			file.clear();
			option_list.clear();
			cache_size_list.clear();
			windows.clear();
			next_access_list.clear();
			cost_res.clear();

			//#pragma omp critical 
			//{
			//	cout << result_files[i] << " done" << endl;
			//}
		}
	}
	
	result_files.clear();
	return;
}


void LoadMemoryTest() {
	Load load;
	char* data;
	long size;
	load.LoadMemory("log/short/trace_fft", data, size);
	cout << data[0] << data[1] << data[2] << data[3] << data[4] << endl;
	return;
}

void log_new_test() {
	Load load;
	vector<int> cache_size;
	for(int i=1; i<=256; i*=2) cache_size.push_back(i);
	map<int, vector<int>> swap_in;
	map<int, vector<int>> swap_out;
	load.LoadLog("log_new/parsec/sampled_parsec/trace_x264", cache_size, swap_in, swap_out);
	for(auto it = cache_size.begin(); it != cache_size.end(); ++it) {
		auto _cache_size = *it;
		cout << _cache_size << " " << (swap_in[_cache_size]).size() << " " << (swap_out[_cache_size]).size() << endl;
	}
	return;
}

int main(int argc, char const *argv[])
{
	const char* result_dir = "result/";
	const char* log_dir = "log_new/";
	const char* rd_dir = "../data/rd/";
	const char* source_dir = "./";
	const char* catalog = "demo/";
	const char* granularity = "demo/";


	bool isOnline = true;
	if ( string(argv[1]) == "offline") isOnline = false;

	// handle_analysis(result_dir,log_dir, rd_dir, source_dir, catalog, granularity,
	handle_analysis(result_dir,log_dir, rd_dir, source_dir, argv[2], argv[3],
	//isOnline
	isOnline,
	//isCustom, only valid when isOnline==false
	false,
	//isResult 
	false, 
	//isOutput
	true,  
	//isDebug
	true);


	// log_new_test();
	// TESTwtinylfu();
	// BeladyCacheTest();
	// BeladyACCacheTest();

/*
	handle_sampling(result_dir,log_dir, rd_dir, "../data/", "sampling_test_cp/", argv[1],
	//isOutput
	true,  
	//isDebug
	true);
*/

/*
	analyze_wtinylfu(result_dir,log_dir, source_dir, catalog, granularity,
	//isOutput
	true,  
	//isDebug
	true);
*/

/*
	custom_cache(result_dir,log_dir, source_dir, catalog, granularity,
	//isOutput
	true,  
	//isDebug
	false);
*/
	// CounterTest();
	return 0;
}
