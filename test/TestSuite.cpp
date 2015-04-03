/**
Copyright (c) 2015 Nuwa Information Co., Ltd, All Rights Reserved.

Distributed under the Boost Software License, Version 1.0.
See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt

See the License for the specific language governing permissions and
limitations under the License.
*/

#include "TestSuite.hpp"
#include "LRUTest.hpp"

test_suite* init_unit_test_suite(int, char*[]) 
{
 test_suite* test = BOOST_TEST_SUITE("LRU Test Suite");

 // Add test functions here.
 test->add(BOOST_TEST_CASE(&TestFib));
 test->add(BOOST_TEST_CASE(&TestFactorial));

 return test;
}
