/**
Copyright (c) 2015 Nuwa Information Co., Ltd, All Rights Reserved.

Distributed under the Boost Software License, Version 1.0.
See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt

See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _NUWAINFO_LRU_TEST_
#define _NUWAINFO_LRU_TEST_

#include "TestSuite.hpp"
#include "../include/LRU.hpp"

using namespace std;

int Fib(int n)
{
 if(n == 0)
  return 0;
 
 if(n == 1)
  return 1;
 
 return Fib(n - 1) + Fib(n - 2);
}

// In test we must duplicate function implemation, to make normal and 
// cached version co-exist.
LRU_DECL1(int, FibCached, int, n)
LRU_CACHED1(int, FibCached, int, n)
{
 if(n == 0)
  return 0;
 
 if(n == 1)
  return 1;
 
 return FibCached(n - 1) + FibCached(n - 2);
}

/**
Test LRU cache for Fibonacci Sequence.
*/
void TestFib()
{
 TimeReporter _t;
 int n;
 int assertN = 102334155; // Fib(40);
 
 _t.Start("Test Normal Fib(40).");
 n = Fib(40);
 _t.End();

 BOOST_CHECK(n == assertN); 
 
 _t.Start("Test LRU Cached Fib(40).");
 n = FibCached(40);
 _t.End(); 
 
 BOOST_CHECK(n == assertN); 
}

struct Factorial
{
 long long Fact(int x) 
 {
  return (x == 1 ? x : x * Fact(x - 1));
 }

 LRU_CACHED1(long long, Fact2, int, x)
 {
  return (x == 1 ? x : x * Fact2(x - 1));
 }
};

/**
Test LRU cache for Factorial with method form.
*/
void TestFactorial()
{
 TimeReporter _t;
 long long n;
 long long assertN = 2432902008176640000; // Fact(20);
 
 Factorial f;
 _t.Start("Test Normal Factorial(20).");
 n = f.Fact(20);
 _t.End();

 BOOST_CHECK(n == assertN); 
 
 _t.Start("Test LRU Cached Factorial(20).");
 n = f.Fact2(20);
 _t.End(); 

 BOOST_CHECK(n == assertN); 
}

#endif
