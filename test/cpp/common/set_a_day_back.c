#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#define SECONDS_IN_A_DAY 86400

int main() {
  struct timeval tv;
  int res = gettimeofday(&tv, NULL);
  if (res) {
    printf("gettimeofday failed: %d\n", errno);
    exit(1);
  }
  long long int second = tv.tv_sec;
  printf("tv_sec: %lld\n", second);
  second -= SECONDS_IN_A_DAY;
  tv.tv_sec = second;
  while (1) {
    res = settimeofday(&tv, NULL);
    if (res) {
      printf("settimeofday failed: %d\n", errno);
      exit(1);
    }
    printf("settimeofday succeeded\n");
    res = gettimeofday(&tv, NULL);
    if (res) {
      printf("gettimeofday failed: %d\n", errno);
      exit(1);
    }
    second = tv.tv_sec;
    printf("tv_sec: %lld\n", second);
    sleep(1);
  }
}
