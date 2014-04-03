g++ -std=c++0x -g -o unittests -DUSE_PARALLEL pthread_permit.c unittests.cpp -lrt -ltbb
if [ "$?" != "0" ]; then
  g++ -std=c++0x -g -o unittests pthread_permit.c unittests.cpp -lrt
fi
g++ -std=c++0x -g -o pthread_permit_speedtest pthread_permit.c pthread_permit_speedtest.cpp -lrt
