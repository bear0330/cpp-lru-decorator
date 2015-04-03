/**
$Id: TestSuite.hpp 107 2012-09-11 14:30:15Z Bear $

Copyright (c) 2012 Nuwa Information Co., Ltd, All Rights Reserved.

Distributed under the Boost Software License, Version 1.0.
See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt

See the License for the specific language governing permissions and
limitations under the License.

$Author: Bear $
$Date: 2012-09-11 22:30:15 +0800 (週二, 11 九月 2012) $
$Revision: 107 $
*/

#ifndef _TEST_SUITE_HEADER_
#define _TEST_SUITE_HEADER_

#ifdef _DEBUG
#ifdef WIN32
// Damn Visual Studio will cause compilation error 
// if we do't include <valarray> first.
#include <valarray>

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif
#endif

#include <iostream>
//#ifdef WIN32
// that will make the compile error while other code used max marco...
//#include <windows.h>
//#else
#include <ctime>
//#endif
#include <string>
#include <sstream>
#include <fstream>

#include <boost/smart_ptr.hpp>

#include <boost/algorithm/string.hpp>

#include <boost/test/unit_test.hpp>

using namespace std;
using namespace boost;
using namespace boost::unit_test;

#ifdef _DEBUG
#ifdef WIN32
#ifndef _WIN32_LEAK_REPORT
/**
Reporter for memory leak.
*/
struct LeakReporter 
{
 static LeakReporter Instance;

 LeakReporter()
 {
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
 }
 
 ~LeakReporter()
 {
  // _CrtDumpMemoryLeaks(); // replaced by the above
 }
};

LeakReporter LeakReporter::Instance; 

#define _WIN32_LEAK_REPORT
#endif
#endif
#endif

/**
Reporter for time elapsed.
*/
struct TimeReporter
{
//#ifdef WIN32
//	LARGE_INTEGER _freq;   
// LARGE_INTEGER _start;
// LARGE_INTEGER _finish;
//#else
 ::clock_t _start, _finish;
//#endif
                            
 void Start(string method = "****************") 
 {
  cout << endl;
  cout << 
   "================== " << method << " ==================";
  cout << endl;
//#ifdef WIN32
//  ZeroMemory(&_freq, sizeof(LARGE_INTEGER));
//  ZeroMemory(&_start, sizeof(LARGE_INTEGER));
//  ZeroMemory(&_finish, sizeof(LARGE_INTEGER));
//
//  QueryPerformanceFrequency(&_freq); 
//  QueryPerformanceCounter(&_start);
//#else
  _start = ::clock();
//#endif
 }

 double Elapsed()
 {
//#ifdef WIN32
//  QueryPerformanceCounter(&_finish);
//  LONGLONG millisecond = (((_finish.QuadPart - _start.QuadPart) * 1000) / _freq.QuadPart);
//  return ((double)millisecond / (double)1000.0);
//#else
  _finish = ::clock();
  return (double)(_finish - _start) / (double)CLOCKS_PER_SEC;
//#endif
 }

 void End(double elapsed = -1)
 {
  cout << endl;
  cout << 
   "======================================================";
  cout << endl;

  if(elapsed == -1)
   elapsed = Elapsed();

  cout << "Elapsed time: " << fixed << elapsed << endl;
 }
};

#endif
