/* test_permit.c
Tests the proposed C1X permit object
(C) 2011-2012 Niall Douglas http://www.nedproductions.biz/


On a 2.67Ghz Intel Core 2 Quad:

Simple permit1:

Uncontended grant time: 48 cycles
Uncontended revoke time: 0 cycles
Uncontended wait time: 142 cycles

1 contended grant time: 725 cycles (359 without revoke)
1 contended revoke time: 4 cycles
1 contended wait time: 1044 cycles (372 without revoke)


DoConsume:

Uncontended grant time: 61 cycles
Uncontended revoke time: 45 cycles
Uncontended wait time: 135 cycles

1 contended grant time: 731 cycles
1 contended revoke time: 160 cycles
1 contended wait time: 1195 cycles


DontConsume:

Uncontended grant time: 102 cycles
Uncontended revoke time: 46 cycles
Uncontended wait time: 137 cycles

1 contended grant time: 856 cycles
1 contended revoke time: 207 cycles
1 contended wait time: 887 cycles
*/

#include "pthread_permit.h"
#ifndef PTHREAD_PERMIT_USE_BOOST
#include <thread>
using namespace std;
#else
#include "boost/thread.hpp"
using namespace boost;
#endif
#include "timing.h"
#include <stdio.h>

#define THREADS 2
#define CYCLESPERMICROSECOND (3.9*1000000000/1000000000000)
#define DONTCONSUME 0
// Define to test uncontended, set to what to exclude (0=test permit_wait, 1=test permit_revoke/permit_grant)
#define UNCONTENDED 1

static usCount timingoverhead;
static thrd_t threads[THREADS];
static atomic<bool> done;
static void *permitaddr;

void mssleep(long ms)
{
  struct timespec ts;
  ts.tv_sec=ms/1000;
  ts.tv_nsec=(ms % 1000)*1000000;
  thrd_sleep(&ts, NULL);
}

template<typename permit_t, int (*permit_grant)(pthread_permitX_t), void (*permit_revoke)(permit_t *), int(*permit_wait)(permit_t *, mtx_t *mtx)> int threadfunc(void *mynum)
{
  size_t mythread=(size_t) mynum;
  permit_t *permit=(permit_t *)permitaddr;
  usCount start, end;
  size_t count=0;
#ifdef UNCONTENDED
  if(UNCONTENDED==mythread) return 0;
#endif
  if(!mynum)
  {
    usCount revoketotal=0, granttotal=0;
    while(!done)
    {
#if DONTCONSUME
      // Revoke permit
      start=GetUsCount();
      permit_revoke(permit);
      end=GetUsCount();
      revoketotal+=end-start-timingoverhead;
      //printf("Thread %u revoked permit\n", mythread);
#endif
      //mssleep(1000);
      //printf("\nThread %u granting permit\n", mythread);
      start=GetUsCount();
      permit_grant(permit);
      end=GetUsCount();
      granttotal+=end-start-timingoverhead;
      //mssleep(1);
      count++;
    }
    printf("Thread %u, average revoke/grant time was %u/%u cycles\n", (unsigned) mythread, (unsigned)((double)revoketotal/count*CYCLESPERMICROSECOND), (unsigned)((double)granttotal/count*CYCLESPERMICROSECOND));
    permit_grant(permit);
  }
  else
  {
    usCount waittotal=0;
    mtx_t mtx;
    mtx_init(&mtx, mtx_plain);
    mtx_lock(&mtx);
    while(!done)
    {
      // Wait on permit
      start=GetUsCount();
      permit_wait(permit, &mtx);
      end=GetUsCount();
      waittotal+=end-start-timingoverhead;
      count++;
      //printf("%u", mythread);
#if defined(UNCONTENDED) && 0==DONTCONSUME
      if(UNCONTENDED==0 && 1==mythread)
      {
        permit_grant(permit);
      }
#endif
    }
    printf("Thread %u, average wait time was %u cycles\n", (unsigned) mythread, (unsigned)((double)waittotal/count*CYCLESPERMICROSECOND));
  }
  return 0;
}

int main(void)
{
  int n;
  usCount start;
  pthread_permit1_t permit1;

  printf("Press key to continue ...\n");
  getchar();
  printf("Wait ...\n");
  start=GetUsCount();
  while(GetUsCount()-start<3000000000000ULL);
  printf("Calculating timing overhead ...\n");
  for(n=0; n<5000000; n++)
  {
    start=GetUsCount();
    timingoverhead+=GetUsCount()-start;
  }
  timingoverhead/=5000000;
  printf("Timing overhead on this machine is %u us. Go!\n", (unsigned) timingoverhead);

  pthread_permit1_init(&permit1, 1);
  permitaddr=&permit1;
  for(n=0; n<THREADS; n++)
  {
    thrd_create(&threads[n], (thrd_start_t) threadfunc<pthread_permit1_t, pthread_permit1_grant, pthread_permit1_revoke, pthread_permit1_wait>, (void *)(size_t)n);
  }
  printf("Press key to kill all\n");
  getchar();
  done=1;
  mssleep(2000);
  printf("Press key to exit\n");
  getchar();
  return 0;
}
