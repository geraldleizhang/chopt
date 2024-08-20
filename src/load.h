#include <iostream>
#include <fstream>
#include <climits>
#include <sys/types.h>
#include <dirent.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <time.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

using namespace std;

class Load {
public:
	Load(){}

	void ReadDirectory(const char* name, vector<string>& v);

	int UniqueNum(vector<string>& file);

	int UniqueNum(vector<string>& file, vector<bool>& option_list, int& cachable_num);

	void LoadFile(const char* filename, vector<string>& file, vector<bool>& option_list);

	void LoadCPFile(const char* filename, vector<string>& file, vector<bool>& option_list);

	void LoadInterval(vector<string>& file, vector<int>& next_access_list, unordered_map<string, int>& frequency, map<string, vector<int>>& item_history);

	int LoadLog(const char* filename, vector<int>& cache_size, map<int, vector<int>>& swap_in, map<int, vector<int>>& swap_out);

	void Shuffle(vector<string>& shuffled_file);

	void LoadMemory(const char* filename, char* &data, long &size);

	void LoadRd(const char* filename, vector<int>& rd);

	void LoadOffline(const char* filename, vector<int>& operation);
};