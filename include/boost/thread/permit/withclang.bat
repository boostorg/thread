copy /y pthread_permit.c pthread_permit.cpp
clang++-3.4 -std=c++11 -fopenmp -fsanitize=undefined -fsanitize=thread -o unittests pthread_permit.cpp unittests.cpp -lrt -lpthread
clang++-3.4 -std=c++11 -fopenmp -o pthread_permit_speedtest pthread_permit.cpp pthread_permit_speedtest.cpp -lrt -lpthread
