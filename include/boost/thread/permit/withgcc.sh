g++-4.8 -std=c++0x -g -o unittests -DUSE_PARALLEL pthread_permit.c unittests.cpp -lrt -ltbb -lpthread
if [ "$?" != "0" ]; then
  g++-4.8 -std=c++0x -g -o unittests pthread_permit.c unittests.cpp -lrt -lpthread
fi
g++-4.8 -std=c++0x -g -o pthread_permit_speedtest pthread_permit.c pthread_permit_speedtest.cpp -lrt -lpthread
