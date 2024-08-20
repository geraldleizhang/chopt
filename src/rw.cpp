#include <iostream>
#include <string>
#include <map>
#include "load.h"

using namespace std;

int main(int argc, char const *argv[])
{
	Load load;
	vector<string> _files;
	load.ReadDirectory("../data/sampled/short/", _files);

	for(int i = 0; i != _files.size(); i++) {
		auto filename = _files[i];
		cout << filename << endl;
		vector<string> file;
		vector<bool> option_list;
		load.LoadFile(("../data/sampled/short/"+filename).c_str(), file, option_list);
		int unique_num = load.UniqueNum(file);
		cout << unique_num << endl;
	}

	return 0;
}