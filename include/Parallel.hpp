/**
$Id: Parallel.hpp 775 2014-02-11 10:53:27Z Bear $

Copyright (c) 2014 Nuwa Information Co., Ltd, All Rights Reserved.

Distributed under the Boost Software License, Version 1.0.
See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt

See the License for the specific language governing permissions and
limitations under the License.

$Author: Bear $
$Date: 2014-02-11 18:53:27 +0800 (週二, 11 二月 2014) $
$Revision: 775 $
*/
#ifndef _NUWAINFO_PARALLEL_
#define _NUWAINFO_PARALLEL_

#include <boost/atomic.hpp>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/locks.hpp>

#include <boost/noncopyable.hpp>

#include <boost/mpl/empty_base.hpp>

#include <boost/shared_ptr.hpp>

namespace LRUImpl
{

// Threading policies, 
// Implementation of the ThreadingModel policy used by various classes,
// Reference:
// http://loki-lib.sourceforge.net/index.php?n=MT.LockingBaseClasses

/**
Implements a single-threaded model; no synchronization.
*/
template<
 typename H, 
 typename M = boost::mpl::empty_base,
 typename L = boost::mpl::empty_base
>
class NullLockable
{
public:

 typedef NullLockable<H, M, L> ThreadPolicyType;

 /// Volatile type.
 typedef H VolatileType;

 /// Mutext type.
 typedef M MutexType;

 /// Lock type.
 typedef L LockType;

public:
 
 /// Dummy Lock class
 struct Lock
 {
  Lock(MutexType& m) 
  {
  }
 };
        
 template<typename T>
 struct Atomic
 {
  typedef T Type;

  static Type& Add(Type& lval, T val)
  {
   return lval += val; 
  }

  static Type& Sub(Type& lval, T val)
  {
   return lval -= val; 
  }

  static Type& Increment(Type& lval)
  { 
   return ++lval; 
  }

  static Type& Decrement(Type& lval)
  { 
   return --lval; 
  }

  static Type& Assign(Type& lval, T val)
  { 
   return lval = val; 
  }
 };

protected:

 /// Dummy mutex.
 mutable M _mutex; 

};

#define __PARALLEL_THREADS_ATOMIC                \
 template<typename T>                            \
 struct Atomic                                   \
 {                                               \
  typedef boost::atomic<T> Type;                 \
                                                 \
  static Type& Add(Type& lval, T val)            \
  {                                              \
   return lval.fetch_add(val);                   \
  }                                              \
                                                 \
  static Type& Sub(Type& lval, T val)            \
  {                                              \
   return lval.fetch_sub(val);                   \
  }                                              \
                                                 \
  static Type& Increment(Type& lval)             \
  {                                              \
   return ++lval;                                \
  }                                              \
                                                 \
  static Type& Decrement(Type& lval)             \
  {                                              \
   return --lval;                                \
  }                                              \
                                                 \
  static Type& Assign(Type& lval, T val)         \
  {                                              \
   return lval.store(val);                       \
  }                                              \
 };

/**  
Implements a object-level locking scheme.
*/
template<
 typename H, 
 typename M = boost::mutex, 
 typename L = boost::unique_lock<M> 
>
class ObjectLevelLockable
{
public:

 typedef ObjectLevelLockable<H, M, L> ThreadPolicyType;

 /// Volatile type.
 typedef volatile H VolatileType;

 /// Mutext type.
 typedef M MutexType;

 /// Lock type.
 typedef L LockType;

public:

 /// Lock.
 typedef LockType Lock;

 __PARALLEL_THREADS_ATOMIC

protected:

 /// Mutex.
 mutable M _mutex;
        
};

/// Handy macro to do object level lock.
#define OBJECT_LEVEL_LOCK \
 typename ThreadPolicyType::Lock _lock(ThreadPolicyType::_mutex);

/**  
Implements a class-level locking scheme.
*/
template<
 typename H, 
 typename M = boost::mutex, 
 typename L = boost::unique_lock<M> 
>
class ClassLevelLockable
{
public:

 typedef ClassLevelLockable<H, M, L> ThreadPolicyType;

 /// Volatile type.
 typedef volatile H VolatileType;

 /// Mutext type.
 typedef M MutexType;

 /// Lock type.
 typedef L LockType;

public:

 /// Lock.
 typedef LockType Lock;

 __PARALLEL_THREADS_ATOMIC

protected:

 /// Mutex.
 static M _mutex;
        
};

/// Handy macro to do object level lock.
#define CLASS_LEVEL_LOCK \
 typename ThreadPolicyType::Lock _lock(ThreadPolicyType::_mutex);

// Handy type defines for C++98 lack of supporting default template value for 
// template template parameter.
// Use C++11 might solve this problem by using or variadic templates.
// Reference: http://stackoverflow.com/questions/5301706/
// default-values-in-templates-with-template-arguments-c

template<typename H>
struct DefaultNullLockable : public NullLockable<H>
{
};

template<typename H>
struct DefaultObjectLevelLockable : public ObjectLevelLockable<H>
{
};

template<typename H>
struct DefaultClassLevelLockable : public ClassLevelLockable<H>
{
};

}

#endif
