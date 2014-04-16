copy /y pthread_permit.c pthread_permit.cpp
del /q unittests.exe
del /q pthread_permit_speedtest.exe
cl /EHsc /Od /Z7 /openmp unittests.cpp pthread_permit.cpp
cl /EHsc /O2 /Z7 /openmp pthread_permit_speedtest.cpp pthread_permit.cpp
