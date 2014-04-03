cp pthread_permit.c pthread_permit.cpp
clang -std=c++11 -o unittests -DUSE_PARALLEL pthread_permit.cpp unittests.cpp -lrt -ltbb
if [ "$?" != "0" ]; then
  clang -std=c++11 -o unittests pthread_permit.cpp unittests.cpp -lrt
fi
clang -std=c++11 -o pthread_permit_speedtest pthread_permit.cpp pthread_permit_speedtest.cpp -lrt
