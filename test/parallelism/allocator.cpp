#include <iostream>
#include <memory>

#include <gtest/gtest.h>
#include <mio/parallelism/allocator.hpp>

TEST(allocator, allocator)
{
    mio::parallelism::allocator<int> alloc_;
    mio::parallelism::allocator<int> alloc2_;

    mio::parallelism::aba_ptr<int> p = alloc_.allocate(1);
    mio::parallelism::aba_ptr<int> p2 = alloc_.allocate(1);
    ASSERT_NE(p.get(), p2.get());

    alloc_.deallocate(p, 1);
    mio::parallelism::aba_ptr<int> p3 = alloc_.allocate(1);
    ASSERT_EQ(p.get(), p3.get());
    ASSERT_EQ(alloc_, alloc_);
    ASSERT_NE(alloc_, alloc2_);
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}