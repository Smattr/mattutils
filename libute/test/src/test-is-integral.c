/// @file
/// @brief Tests of `is_integral`
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <ute/type_traits.h>

TEST("is_integral") {
  ASSERT_EQ(is_integral(bool), 1);
  ASSERT_EQ(is_integral(char), 1);
  ASSERT_EQ(is_integral(signed char), 1);
  ASSERT_EQ(is_integral(unsigned char), 1);
  ASSERT_EQ(is_integral(int), 1);
  ASSERT_EQ(is_integral(unsigned int), 1);
  ASSERT_EQ(is_integral(short), 1);
  ASSERT_EQ(is_integral(unsigned short), 1);
  ASSERT_EQ(is_integral(long), 1);
  ASSERT_EQ(is_integral(unsigned long), 1);
  ASSERT_EQ(is_integral(long long), 1);
  ASSERT_EQ(is_integral(unsigned long long), 1);
  ASSERT_EQ(is_integral(const int), 1);
  ASSERT_EQ(is_integral(double), 0);
  ASSERT_EQ(is_integral(char *), 0);
}
