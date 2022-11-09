#include <pthread.h>

#include <gtest/gtest.h>

#include "test/core/util/test_config.h"

namespace {

int _global = 0;
void* Thread1(void* arg) {
  _global++;
  return NULL;
}

TEST(ThreadLeakTest, BasicTest) {
  pthread_t t;
  pthread_create(&t, NULL, Thread1, NULL);
}

}  // namespace

int main(int argc, char** argv) {
  grpc::testing::TestEnvironment env(&argc, argv);
  ::testing::InitGoogleTest(&argc, argv);
  int ret = RUN_ALL_TESTS();
  return ret;
}
