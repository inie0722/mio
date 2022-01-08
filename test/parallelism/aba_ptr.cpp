#include <iostream>
#include <gtest/gtest.h>
#include <mio/parallelism/aba_ptr.hpp>
#include <memory>
#include <boost/container/vector.hpp>

TEST(aba_ptr, aba_ptr) 
{
    int v = rand();
    mio::parallelism::aba_ptr<int> p(&v);

    ASSERT_EQ(p.get(), &v);
    ASSERT_EQ(p, &v);
    ASSERT_EQ(*p, v);
    ASSERT_EQ(p[0], v);
    ASSERT_NE(p, nullptr);
    ASSERT_TRUE((bool)p == true);

    auto c = (mio::parallelism::aba_ptr<double>)p;
    ASSERT_NE(c, nullptr);
    ASSERT_TRUE(c);
}

TEST(atomic, load) 
{
  int v;
  mio::parallelism::aba_ptr<int> p(&v);
  std::atomic<mio::parallelism::aba_ptr<int>> atomic_p(p);
  
  ASSERT_EQ(p, atomic_p.load());
}

TEST(atomic, store) 
{
  int v;
  mio::parallelism::aba_ptr<int> p(&v);
  std::atomic<mio::parallelism::aba_ptr<int>> atomic_p(p);
  
  atomic_p.store(p);
  ASSERT_NE(p, atomic_p);
  ASSERT_EQ(p.get(), atomic_p.load().get());
}

TEST(atomic, exchange) 
{
  int v;
  mio::parallelism::aba_ptr<int> p(&v);
  std::atomic<mio::parallelism::aba_ptr<int>> atomic_p(p);
  
  ASSERT_EQ(p, atomic_p.exchange(p));
  ASSERT_NE(p, atomic_p);
}

TEST(atomic, compare_exchange_weak) 
{
  int v[2];
  mio::parallelism::aba_ptr<int> p1(&v[0]);
  mio::parallelism::aba_ptr<int> p2(&v[1]);
  std::atomic<mio::parallelism::aba_ptr<int>> atomic_p(p1);
  
  ASSERT_FALSE(atomic_p.compare_exchange_weak(p2, p1));
  ASSERT_EQ(p1, p2);
  p2 = &v[1];
  ASSERT_TRUE(atomic_p.compare_exchange_weak(p1, p2));
  ASSERT_NE(p1, p2);
}

TEST(atomic, compare_exchange_strong) 
{
  int v[2];
  mio::parallelism::aba_ptr<int> p1(&v[0]);
  mio::parallelism::aba_ptr<int> p2(&v[1]);
  std::atomic<mio::parallelism::aba_ptr<int>> atomic_p(p1);
  
  ASSERT_FALSE(atomic_p.compare_exchange_strong(p2, p1));
  ASSERT_EQ(p1, p2);
  p2 = &v[1];
  ASSERT_TRUE(atomic_p.compare_exchange_strong(p1, p2));
  ASSERT_NE(p1, p2);
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();   
}