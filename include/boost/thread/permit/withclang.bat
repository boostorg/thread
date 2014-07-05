copy /y pthread_permit.c pthread_permit.cpp
clang -std=c++11 -o unittests -DUSE_PARALLEL -I../intel_tbb/include pthread_permit.cpp unittests.cpp -lpthread -L ../intel_tbb/lib -ltbb_debug
if ERRORLEVEL 1 clang -std=c++11 -o unittests pthread_permit.cpp unittests.cpp -lpthread
clang -std=c++11 -o pthread_permit_speedtest pthread_permit.cpp pthread_permit_speedtest.cpp -lpthread
