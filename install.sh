rm -rf main
g++ -g -O3 -std=c++11 -I src/ -pthread -fopenmp -o main main.cpp src/kcolor.cpp src/kcolor.h src/load.cpp src/load.h src/mcmf.h src/mcmf.cpp src/mcmf_new.h
rm -rf analysis
g++ -g -O3 -std=c++11 -I src/ -fopenmp -o analysis analysis.cpp src/load.h src/load.cpp src/lru.h src/lfu.h src/belady.h src/beladyAC.h src/static.h src/tinylfu.h src/bf.h src/cbf.h src/slru.h src/wtinylfu.h src/tinylfu_only.h src/monitor.h src/custom.h
