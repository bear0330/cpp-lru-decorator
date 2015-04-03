# cpp-lru-decorator
Header only LRU decorator for C++03 based on boost::bimaps (algorithm heavily based on Tim Day's work: http://timday.bitbucket.org/lru.html).

It let you can simply change function signature to add LRU cache to a function or method.

# Features
  * Simple to use and revert.
  * Automatically remember combination of arguments to return cached result.
  * Header only.
  * Support multithreading via simply define LRU_DEFAULT_LOCK_LEVEL.
  
# Usage

  * For normal function, use LRU_DECL# and LRU_CACHED# macro where # is parameter number. For example, `int f(char v1, float v2)` changes to `LRU_DECL2(int, f, char, float)` and `LRU_CACHED2(int, f, char, float)`.
  * For method, only LRU_CACHED# macro required.
  * #define LRU_DEFAULT_LOCK_LEVEL if required; 0 means no lock used, 1 means object level lock used, 2 is class level lock. Default value is 1 for safety. If you run a single-threaded program, you can use 0 to improve performance.
  * For every function, capacity is default to 4096, you can define LRU_DEFAULT_CAPACITY to change it.

Example:
```
#include "LRU.hpp"

//int Fib(int n) // This is oringnal function signature, you simply change it to:
LRU_DECL1(int, Fib, int, n)
LRU_CACHED1(int, Fib, int, n)
{
 if(n == 0)
  return 0;
 
 if(n == 1)
  return 1;
 
 return Fib(n - 1) + Fib(n - 2);
}
```

On my machine (Intel i5 2400 3.10GHz), orignal Fib(50) run **93.18 seconds**, LRU cached version run **0.002 seconds**.

Example2: (method)
```
#include "LRU.hpp"

struct A
{
 // int Method1(int p) // Only LRU_CACHED required.
 LRU_CACHED1(int, Method1, int, p)
 {
  // ... implemetation.
 }
};
```

# Limitations
  * Maximum 10 parameters supported.
  * Return type void is not supported. (LRU expect cache the result, if no result, no cache required.)
  * Function without parameter is not supported. (Without the parameter, a function suppose to return the same result every time, you can do a simple cache by yourself.)
  * Tested with Boost 1.43, Microsoft Visual Studio 2010 and GCC 4.4. Higher version of Boost, Visual Studio and GCC should work also.
