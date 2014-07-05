g++ -std=c++0x -g -o unittests -DUSE_PARALLEL -I../intel_tbb/include pthread_permit.c unittests.cpp -lpthread -L ../intel_tbb/lib -ltbb_debug
if ERRORLEVEL 1 g++ -std=c++0x -g -o unittests pthread_permit.c unittests.cpp -lpthread
g++ -std=c++0x -g -o pthread_permit_speedtest pthread_permit.c pthread_permit_speedtest.cpp -lpthread
