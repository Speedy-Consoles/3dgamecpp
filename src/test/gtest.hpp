#ifndef GTEST_HPP_
#define GTEST_HPP_

#include <gtest/gtest.h>

#define ASSERT_NO_DEATH(stmt) ASSERT_EXIT({stmt exit(0);}, ExitedWithCode(0), ".*")

#endif // GTEST_HPP_
