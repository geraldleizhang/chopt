#include <load.h>
#include <time.h>

void Load::ReadDirectory(const char* name, vector<string>& v)
{
    DIR* dirp = opendir(name);
    if (dirp == NULL) {
    	cout << "Error opening " << name << endl;
    	return;
    }
    struct dirent * dp;
    while ((dp = readdir(dirp)) != NULL) {
        if(int(dp->d_name[0]) != 46) {
        	string temp = string(dp->d_name);
        	if( temp.find("_info") == string::npos ) v.push_back(string(dp->d_name));
        }
    }
    closedir(dirp);
    return;
}

int Load::UniqueNum(vector<string>& file) {
	map<string, int> counter;
	for(auto it = file.begin(); it != file.end(); ++it) {
		counter[*it] = 1;
	}

	return counter.size();
}

int Load::UniqueNum(vector<string>& file, vector<bool>& option_list, int& cachable_num) {
	map<string, int> counter;
	cachable_num = 0;
	for(int i=0; i<file.size(); i++) {
		counter[file[i]] += option_list[i]? 5 : 1;
	}

	for(auto it = counter.begin(); it != counter.end(); ++it) {
		if(it->second > 10) cachable_num += 1;
	}

	return counter.size();
}

void Load::LoadFile(const char* filename, vector<string>& file, vector<bool>& option_list) {
	/*ifstream _in;
	_in.open(filename);
	string line;
	int count = 0;
	while(getline(_in, line)) {
		int pos = line.find(",");
		string key = line.substr(0, pos);
		if (key == "page") continue;
		//if(key == "2025") continue;
		string option = line.substr(line.size()-1,line.size());
		if (option != "w" && option != "r") {
			cout << key << " " << option << endl;
			cout << "No read/write option" << endl;
		} 
		bool isWrite = false;
		if(option == "w") isWrite = true;
		
		file.push_back(key);
		option_list.push_back(isWrite);
		count++;
		if(count == 100000000) break;
	}

	_in.close();*/


	char* data;
	long size;
	LoadMemory(filename, data, size);

	long start = 0;
	long end = 0;
	char* sentence = new char[100];
	string line;
	int count = 0;
	bool num_ready = true;
	
	for(long i=0; i<size; i++) {		
		auto item = data[i];
		if( item == '\n') {
			end = i;
			num_ready = true;
			strncpy(sentence, data+start, end-start);
			line.assign(sentence, end-start);
			start = i+1;
		} else {
			num_ready = false;
			continue;
		}

		if(num_ready) {

			int pos = line.find(",");
			string key = line.substr(0, pos);
			if (key == "page") continue;
			string option = line.substr(line.size()-1,line.size());
			if (option != "w" && option != "r") {
				cout << key << " " << option << endl;
				cout << "No read/write option" << endl;
			}
			
			bool isWrite = false;
			if(option == "w") isWrite = true;
			
			file.push_back(key);
			option_list.push_back(isWrite);
			count++;
		}
		if(count == 100000000) break;
	}

	return;
}

void Load::LoadCPFile(const char* filename, vector<string>& file, vector<bool>& option_list) {
	char* data;
	long size;
	LoadMemory(filename, data, size);
	
	long start = 0;
	long end = 0;
	char* sentence = new char[100];
	string line;
	int count = 0;
	bool num_ready = true;
	
	for(long i=0; i<size; i++) {		
		auto item = data[i];
		if( item == '\n' ) {
			end = i;
			num_ready = true;
			strncpy(sentence, data+start, end-start);
			line.assign(sentence, end-start);
			start = i+1;
		} else {
			num_ready = false;
			continue;
		}

		if(num_ready) {
			int pos = line.find(",");
			string key = line.substr(0, pos);
			if (key == "page") continue;
			string option = line.substr(line.size()-1,line.size());
			if (option != "w" && option != "r") {
				cout << key << " " << option << endl;
				cout << "No read/write option" << endl;
			}
			
			bool isWrite = false;
			if(option == "w") isWrite = true;
			file.push_back(key);
			option_list.push_back(isWrite);
			count++;
		}
	}
	
	return;
}

void Load::Shuffle(vector<string>& shuffled_file) {
	srand((unsigned)time(NULL));  
	for(int i = shuffled_file.size()-1; i >= 0; --i) {
		auto index = rand() % i;
		auto temp = shuffled_file[index];
		shuffled_file[index] = shuffled_file[i];
		shuffled_file[i] = temp;
	}
	return;
}

void Load::LoadInterval(vector<string>& file, vector<int>& interval_list, unordered_map<string, int>& frequency, map<string, vector<int>>& item_history) {
	string line;

	vector<string> trace;
	map<string, int> last_occured;

	int count = 0;

	for(auto it = file.begin(); it != file.end(); ++it) {

		auto key = *it;
		
		// if already in last_occured, update the last occured with current count
		if (last_occured.count(key) > 0) {
			interval_list[last_occured[key]] = count;
		}
		// else, the key occurs first time
		last_occured[key] = count;
		// always push back INT_MAX of the trace
		interval_list.push_back(INT_MAX);

		frequency[key] += 1; 

		count++;
		item_history[key].push_back(count);
	}
	last_occured.clear();
	trace.clear();

	return;
}

void Load::LoadMemory(const char* filename, char* &data, long &size) {
	int fd=open(filename,O_RDONLY); 
	size = lseek(fd, 0, SEEK_END);
	data = (char *) mmap( NULL,  size ,PROT_READ, MAP_PRIVATE, fd, 0 ); 
	return;
}

int Load::LoadLog(const char* filename, vector<int>& cache_size, map<int, vector<int>>& swap_in, map<int, vector<int>>& swap_out){
	char* data;
	long size;
	LoadMemory(filename, data, size);

	int num = 0;
	bool num_ready = false;
	int cache_size_index = 0;

	int input = 0;
	int state = 0;
	int round = 0;
	bool isTarget = false;

	for(long i=0; i<size; i++) {		
		auto item = data[i];
		if( item == ' ' || item == '\n' ) {		
			if(num_ready == false) {
				num_ready = true;
				input = num;
				num = 0;
			} else {
				num_ready = false;
			}
		
			if(item == '\n' && state){
				state = ( state + 1 ) % 3;
			} 
		} else {
			num_ready = false;
			int temp = item - '0';
			num = num*10 + temp; 
			continue;
		}

		if(num_ready) {
			switch(state){
				case 0:
					round = input+1;
					isTarget = false;
					if(round == cache_size[cache_size_index]) {
						isTarget = true;
						cache_size_index++;
					}
					state++;
					break;
				case 1:
					if(isTarget) swap_in[round].push_back(input-1);
					break;
				case 2:
					if(isTarget) swap_out[round].push_back(input-1);
					break;
			}
		}
	}

	return round;
}

void Load::LoadRd(const char* filename, vector<int>& rd) {
	ifstream _in;
	_in.open(filename);
	string line;
	while(getline(_in, line)) {		
		rd.push_back(stoi(line));
	}

	_in.close();
	return;
}

void Load::LoadOffline(const char* filename, vector<int>& operation) {
	char* data;
	long size;
	LoadMemory(filename, data, size);
	
	long start = 0;
	long end = 0;
	char* sentence = new char[100];
	string line;
	int count = 0;
	bool num_ready = true;
	
	for(long i=0; i<size; i++) {		
		auto item = data[i];
		if(int(item) == 49) operation.push_back(1);
		else if (int(item) == 48) operation.push_back(0);
		else continue;
	}
	
	return;
}