g++-4.8 -std=c++0x -O0 -g -fopenmp -o unittests pthread_permit.c unittests.cpp -lrt -lpthread
g++-4.8 -std=c++0x -O3 -g -fopenmp -o pthread_permit_speedtest pthread_permit.c pthread_permit_speedtest.cpp -lrt -lpthread
