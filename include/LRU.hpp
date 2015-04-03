/**
$Id: LRU.hpp 773 2014-02-10 06:48:20Z Bear $

Copyright (c) 2014 Nuwa Information Co., Ltd, All Rights Reserved.

Distributed under the Boost Software License, Version 1.0.
See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt

See the License for the specific language governing permissions and
limitations under the License.

$Author: Bear $
$Date: 2014-02-10 14:48:20 +0800 (週一, 10 二月 2014) $
$Revision: 773 $
*/
#ifndef _NUWAINFO_LRU_
#define _NUWAINFO_LRU_

#include <boost/bimap.hpp>
#include <boost/bimap/list_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>

#include <boost/function.hpp>

#include <boost/type_traits.hpp>
#include <boost/typeof/typeof.hpp>

#include <boost/pool/detail/singleton.hpp> 

#include <boost/unordered_map.hpp>

#include <boost/any.hpp>

#include <boost/preprocessor/repetition.hpp>
#include <boost/preprocessor/iteration/local.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/slot/slot.hpp>

#include <boost/shared_ptr.hpp>

#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>

#include "Patch/boost/tuple/detail/hash_tuple.hpp"

#include "Parallel.hpp"

#ifndef LRU_DEFAULT_CAPACITY
#define LRU_DEFAULT_CAPACITY   4096
#endif

#ifndef LRU_DEFAULT_LOCK_LEVEL
#define LRU_DEFAULT_LOCK_LEVEL 1
#endif

#if LRU_DEFAULT_LOCK_LEVEL == 0
#define LRU_DEFAULT_LOCK_IMPL          NullLockable
#define LRU_DEFAULT_DEFAULT_LOCK_IMPL  DefaultNullLockable
#elif LRU_DEFAULT_LOCK_LEVEL == 1
#define LRU_DEFAULT_LOCK_IMPL          ObjectLevelLockable
#define LRU_DEFAULT_DEFAULT_LOCK_IMPL  DefaultObjectLevelLockable
#else
#define LRU_DEFAULT_LOCK_IMPL          ClassLevelLockable
#define LRU_DEFAULT_DEFAULT_LOCK_IMPL  DefaultClassLevelLockable
#endif

namespace LRUImpl
{

/**
Fixed-size (by number of records) LRU-replacement cache.
Reference: 
http://timday.bitbucket.org/lru.html
http://patrickaudley.com/code/project/lrucache

@param K Key type.
@param V Value type.
@param C Capacity.
@param ThreadPolicy Thread policy class.
*/
template<
 typename K, 
 typename V, 
 std::size_t C = LRU_DEFAULT_CAPACITY, 
 template <class> class ThreadPolicy = DefaultNullLockable
> 
class LRU : public ThreadPolicy<LRU<K, V, C, ThreadPolicy> >
{
public:

#ifdef __GNUG__
  typedef ThreadPolicy<LRU<K, V, C, ThreadPolicy> > ThreadPolicyType;
#endif

  typedef K KeyType;
  typedef V ValueType;
  
  typedef boost::bimaps::bimap<
   boost::bimaps::unordered_set_of<K>,
   boost::bimaps::list_of<V>
  > ContainerType;
  
  typedef boost::function<V(const K&)> FunctionType;
  
public:    

  /**
  Constuctor specifies the cached function and the maximum number of records 
  to be stored.  
  
  @param c Maximum number of records.
  @param f Value store function.  
  */
  LRU(const std::size_t c = C, const FunctionType f = 0) : capacity(c), fn(f)
  {
  }
  
  /**
  Obtain value of the cached function for k. 
  
  Return stored pointer for performance consideration (avoid data copy),
  but it must keep in mind that pointer might invalid after cache update.
  If you want a safer operation, please use Get method or keep a copy of 
  data by yourself.
  
  @param k Key.
  @param found A boolean indicator to indicate whether we found key or not.
  @return Value pointer which stored in cache.
  */
  V* operator()(const K& k, bool* found = NULL) 
  {
   OBJECT_LEVEL_LOCK;

   // Attempt to find existing record.
   typename ContainerType::left_iterator it = container.left.find(k);

   if(it == container.left.end()) 
   {
    if(found)
     *found = false;

    // We don't have it:
    // Evaluate function and create new record.
    if(fn)
    {
     Put(k, fn(k));
     
     it = container.left.find(k);
     return &it->second;
    }
    else
    {
     return NULL;
    }
   } 

   if(found)
    *found = true;

   // We do have it.
   Update(it);
      
   // Return the retrieved value.
   return &it->second;
  }
  
  /**
  Put value into cache.
  
  @param k Key.
  @param v Value.
  */
  void Put(const K& k,const V& v) 
  {
   if(capacity == 0) /* Disabled */
    return;
        
   OBJECT_LEVEL_LOCK;

   // If necessary, make space.
   if(container.size() == capacity) 
   {
    // By purging the least-recently-used element.
    container.right.erase(container.right.begin());
   }
    
   // Create a new record from the key and the value bimap's list_view 
   // defaults to inserting this at the list tail 
   // (considered most-recently-used).
   container.insert(typename ContainerType::value_type(k, v));
  }  
  
  /**
  Get value from cache.
  
  @param k Key.
  @param default Return value if cache missed.
  @param found A boolean indicator to indicate whether we found key or not.
  @return Value.
  */
  V Get(const K& k, V* _default = NULL, bool* found = NULL)
  {
   OBJECT_LEVEL_LOCK;

   // Attempt to find existing record.
   const typename ContainerType::left_iterator it = container.left.find(k);

   if(it == container.left.end()) 
   {
    if(found)
     *found = false;

    // We don't have it:
    // Evaluate function and create new record
    if(fn)
    {
     const V v = fn(k);     
     Put(k, v);
     
     return v;
    }
    else if(_default)
    {
     return *_default;
    }
    else
    {
     return V();
    }
   } 

   if(found)
    *found = true;

   // We do have it:
   Update(it);
      
   // Return the retrieved value.
   return it->second;
  }  
  
  /**
  Obtain the cached keys, most recently used element at head, 
  least recently used at tail.
  
  @param dst Result container iterator.
  */
  template<typename IT> 
  void GetKeys(IT& dst) const 
  {
   OBJECT_LEVEL_LOCK;

   typename ContainerType::right_const_reverse_iterator src =
    container.right.rbegin();
    
   while(src != container.right.rend()) 
    *dst++ = (*src++).second;
  }
  
  /**
  Cached key exists or not?
  
  @param key Key.
  @return True if exists.
  */
  inline bool Exists(const K& key) const
  {  
   OBJECT_LEVEL_LOCK;

   return container.left.find(key) != container.left.end();
  }

  /**
  Resize capacity.

  @param size Capacity.
  */
  inline void Resize(const std::size_t size)
  {
   capacity = size;
  }
  
private:
 
 /**
 Update internal container.
 
 @param it Retrived value iterator.
 */
 inline void Update(const typename ContainerType::left_iterator& it)
 {
  // Update the access record view.
  container.right.relocate(container.right.end(), 
                           container.project_right(it));                           
 }
  

private:
  
 /// Value store function.
 const FunctionType fn;
  
 /// Capacity (maximum records).
 const std::size_t capacity;
 
 /// Internal container.
 ContainerType container;
 
};

/**
Internal LRUPool to cache all LRU cache objects by function unique key.
This is a helper to provide central control mechanism of all function caches.
It should not be used directly.
*/
class _LRUPool : public LRU_DEFAULT_LOCK_IMPL<_LRUPool>
{
public:

 /// Pool storage, key: function unique id, value: LRU object.
 typedef boost::unordered_map<int, boost::any> PoolTable;

public:

 /**
 Constructor.

 @param c Capacity for all caches.
 */
 _LRUPool(const std::size_t c = LRU_DEFAULT_CAPACITY) : capacity(c)
 {
 }

 // Helper macro.
 #define _LRUPOOL_TUPLE_REMOVE_QUALIFIERS(z, i, var)                           \
  typename boost::remove_const<                                                \
   typename boost::remove_reference<T##i>::type>::type,

 /**
 Get cache value by function unique key and its arguments.
 
 @param funcUnique Function unique key generated by compiler.
 @param found Output reference to indicate cache found or not.
 @param ... Function arguments.
 @return Cached result or default value. 
 */
 #define _LRUPOOL_GET(z, n, unused)                                           \
 template<typename TR, BOOST_PP_ENUM_PARAMS(n, typename T)>                   \
 TR Get(int funcUnique, bool& found, BOOST_PP_ENUM_BINARY_PARAMS(n, T, &t))   \
 {                                                                            \
  typedef boost::tuple<                                                       \
   BOOST_PP_REPEAT(BOOST_PP_SUB(n, 1), _LRUPOOL_TUPLE_REMOVE_QUALIFIERS, T)   \
   typename boost::remove_const<                                              \
    typename boost::remove_reference<                                         \
     BOOST_PP_CAT(T, BOOST_PP_SUB(n, 1))>::type                               \
    >::type                                                                   \
  > ArgsTuple;                                                                \
  typedef LRU<ArgsTuple, TR,                                                  \
   LRU_DEFAULT_CAPACITY,                                                      \
   LRU_DEFAULT_DEFAULT_LOCK_IMPL                                              \
  > Cache;                                                                    \
  typedef boost::shared_ptr<Cache> CachePtr;                                  \
                                                                              \
  found = false;                                                              \
                                                                              \
  if(capacity == 0) /* Disabled */                                            \
   return TR();                                                               \
                                                                              \
  OBJECT_LEVEL_LOCK;                                                          \
                                                                              \
  PoolTable::iterator i = pool.find(funcUnique);                              \
  if(i != pool.end())                                                         \
  {                                                                           \
   CachePtr cache = boost::any_cast<CachePtr>(i->second);                     \
                                                                              \
   ArgsTuple tuple(boost::make_tuple(BOOST_PP_ENUM_PARAMS(n, t)));            \
   if(cache->Exists(tuple))                                                   \
   {                                                                          \
    found = true;                                                             \
    return cache->Get(tuple);                                                 \
   }                                                                          \
  }                                                                           \
  else                                                                        \
  {                                                                           \
   pool[funcUnique] = CachePtr(new Cache(capacity));                          \
  }                                                                           \
                                                                              \
  return TR();                                                                \
 }

 #define BOOST_PP_LOCAL_MACRO(n) _LRUPOOL_GET(~, n, ~)
 #define BOOST_PP_LOCAL_LIMITS   (1, 10)
 #include BOOST_PP_LOCAL_ITERATE()

 /**
 Put function result into cache by function unique key and its arguments.
 
 @param funcUnique Function unique key generated by compiler.
 @param r Function result.
 @param ... Function arguments.
 */
 #define _LRUPOOL_PUT(z, n, unused)                                           \
 template<typename TR, BOOST_PP_ENUM_PARAMS(n, typename T)>                   \
 void Put(int funcUnique, TR& r, BOOST_PP_ENUM_BINARY_PARAMS(n, T, &t))       \
 {                                                                            \
  typedef boost::tuple<                                                       \
   BOOST_PP_REPEAT(BOOST_PP_SUB(n, 1), _LRUPOOL_TUPLE_REMOVE_QUALIFIERS, T)   \
   typename boost::remove_const<                                              \
    typename boost::remove_reference<                                         \
     BOOST_PP_CAT(T, BOOST_PP_SUB(n, 1))>::type                               \
    >::type                                                                   \
  > ArgsTuple;                                                                \
  typedef LRU<ArgsTuple, TR,                                                  \
   LRU_DEFAULT_CAPACITY,                                                      \
   LRU_DEFAULT_DEFAULT_LOCK_IMPL                                              \
  > Cache;                                                                    \
  typedef boost::shared_ptr<Cache> CachePtr;                                  \
                                                                              \
  if(capacity == 0) /* Disabled */                                            \
   return;                                                                    \
                                                                              \
  ArgsTuple tuple(boost::make_tuple(BOOST_PP_ENUM_PARAMS(n, t)));             \
                                                                              \
  OBJECT_LEVEL_LOCK;                                                          \
                                                                              \
  PoolTable::iterator i = pool.find(funcUnique);                              \
  if(i != pool.end())                                                         \
  {                                                                           \
   CachePtr cache = boost::any_cast<CachePtr>(i->second);                     \
   cache->Put(tuple, r);                                                      \
  }                                                                           \
  else                                                                        \
  {                                                                           \
   CachePtr cache(new Cache(capacity));                                       \
                                                                              \
   cache->Put(tuple, r);                                                      \
   pool[funcUnique] = cache;                                                  \
  }                                                                           \
 }

 #define BOOST_PP_LOCAL_MACRO(n) _LRUPOOL_PUT(~, n, ~)
 #define BOOST_PP_LOCAL_LIMITS   (1, 10)
 #include BOOST_PP_LOCAL_ITERATE()

 /**
 Configure capacity or other parameters for caches.

 @param size Capacity for all caches.
 */
 void Configure(const std::size_t size)
 {
  capacity = size;
 }

private:

 /// Pool storate instance.
 PoolTable pool;

 /// Capacity for all caches.
 std::size_t capacity;

};

/// LRUPool public typedef, singleton.
typedef boost::details::pool::singleton_default<_LRUPool> LRUPool;

}

#ifdef LRU_DISABLED
#define _LRU_CACHED_IMPL(TRet, TFunc, ... )
#else

// Function Unique can be produced by these and its combination:
//
// http://boost.2283326.n4.nabble.com/
// Preprocessor-library-inrement-a-macro-td2560393.html
// http://alexander-stoyan.blogspot.tw/2012/07/
// getting-pseudo-random-numbers-at.html
// 
// But we keep it as simple as possible.

#define _LRU_CACHED_IMPL(TRet, TFunc, ... )                                 \
 bool found;                                                                \
                                                                            \
 int funcUnique = 0x1e3f75a9 + __COUNTER__;                                 \
 TRet ret = LRUImpl::LRUPool::instance().Get<TRet>(funcUnique, found,       \
                                                   __VA_ARGS__);            \
 if(found)                                                                  \
  return ret;                                                               \
                                                                            \
 TRet ret2 = TFunc##Impl(__VA_ARGS__);                                      \
                                                                            \
 LRUImpl::LRUPool::instance().Put(funcUnique, ret2, __VA_ARGS__);           \
 return ret2;

#endif

// LRU_CACHED Decorators, support up to 10 arguments.
// TODO: Eliminate LRU_DECL macros for normal function usage.
#define LRU_DECL1(TRet, TFunc, T0, A0)                                      \
 TRet TFunc##Impl(T0 A0);

#define LRU_DECL2(TRet, TFunc, T0, A0, T1, A1)                              \
 TRet TFunc##Impl(T0 A0, T1 A1);

#define LRU_DELC3(TRet, TFunc, T0, A0, T1, A1, T2, A2)                      \
 TRet TFunc##Impl(T0 A0, T1 A1, T2 A2);

#define LRU_DELC4(TRet, TFunc, T0, A0, T1, A1, T2, A2, T3, A3)              \
 TRet TFunc##Impl(T0 A0, T1 A1, T2 A2, T3 A3);

#define LRU_DELC5(TRet, TFunc, T0, A0, T1, A1, T2, A2, T3, A3, T4, A4)      \
 TRet TFunc##Impl(T0 A0, T1 A1, T2 A2, T3 A3, T4 A4);

#define LRU_DELC6(TRet, TFunc, T0, A0, T1, A1, T2, A2, T3, A3, T4, A4,      \
                  T5, A5)                                                   \
 TRet TFunc##Impl(T0 A0, T1 A1, T2 A2, T3 A3, T4 A4, T5 A5);

 #define LRU_DELC7(TRet, TFunc, T0, A0, T1, A1, T2, A2, T3, A3, T4, A4,     \
                   T5, A5, T6, A6)                                          \
 TRet TFunc##Impl(T0 A0, T1 A1, T2 A2, T3 A3, T4 A4, T5 A5, T6 A6);

 #define LRU_DELC8(TRet, TFunc, T0, A0, T1, A1, T2, A2, T3, A3, T4, A4,     \
                   T5, A5, T6, A6, T7, A7)                                  \
 TRet TFunc##Impl(T0 A0, T1 A1, T2 A2, T3 A3, T4 A4, T5 A5, T6 A6, T7 A7);

#define LRU_DELC9(TRet, TFunc, T0, A0, T1, A1, T2, A2, T3, A3, T4, A4,      \
                  T5, A5, T6, A6, T7, A7, T8, A8)                           \
 TRet TFunc##Impl(T0 A0, T1 A1, T2 A2, T3 A3, T4 A4, T5 A5, T6 A6, T7 A7,   \
                  T8 A8);

#define LRU_DELC10(TRet, TFunc, T0, A0, T1, A1, T2, A2, T3, A3, T4, A4,     \
                   T5, A5, T6, A6, T7, A7, T8, A8, T9, A9)                  \
 TRet TFunc##Impl(T0 A0, T1 A1, T2 A2, T3 A3, T4 A4, T5 A5, T6 A6, T7 A7,   \
                  T8 A8, T9 A9);

#define LRU_CACHED1(TRet, TFunc, T0, A0)                                    \
 TRet TFunc(T0 A0)                                                          \
 {                                                                          \
  _LRU_CACHED_IMPL(TRet, TFunc, A0)                                         \
 }                                                                          \
 TRet TFunc##Impl(T0 A0)

#define LRU_CACHED2(TRet, TFunc, T0, A0, T1, A1)                            \
 TRet TFunc(T0 A0, T1 A1)                                                   \
 {                                                                          \
  _LRU_CACHED_IMPL(TRet, TFunc, A0, A1)                                     \
 }                                                                          \
 TRet TFunc##Impl(T0 A0, T1 A1)

#define LRU_CACHED3(TRet, TFunc, T0, A0, T1, A1, T2, A2)                    \
 TRet TFunc(T0 A0, T1 A1, T2 A2)                                            \
 {                                                                          \
  _LRU_CACHED_IMPL(TRet, TFunc, A0, A1, A2)                                 \
 }                                                                          \
 TRet TFunc##Impl(T0 A0, T1 A1, T2 A2)

#define LRU_CACHED4(TRet, TFunc, T0, A0, T1, A1, T2, A2, T3, A3)            \
 TRet TFunc(T0 A0, T1 A1, T2 A2, T3 A3)                                     \
 {                                                                          \
  _LRU_CACHED_IMPL(TRet, TFunc, A0, A1, A2, A3)                             \
 }                                                                          \
 TRet TFunc##Impl(T0 A0, T1 A1, T2 A2, T3 A3)

#define LRU_CACHED5(TRet, TFunc, T0, A0, T1, A1, T2, A2, T3, A3, T4, A4)    \
 TRet TFunc(T0 A0, T1 A1, T2 A2, T3 A3, T4 A4)                              \
 {                                                                          \
  _LRU_CACHED_IMPL(TRet, TFunc, A0, A1, A2, A3, A4)                         \
 }                                                                          \
 TRet TFunc##Impl(T0 A0, T1 A1, T2 A2, T3 A3, T4 A4)

#define LRU_CACHED6(TRet, TFunc, T0, A0, T1, A1, T2, A2, T3, A3, T4, A4,    \
                    T5, A5)                                                 \
 TRet TFunc(T0 A0, T1 A1, T2 A2, T3 A3, T4 A4, T5 A5)                       \
 {                                                                          \
  _LRU_CACHED_IMPL(TRet, TFunc, A0, A1, A2, A3, A4, A5)                     \
 }                                                                          \
 TRet TFunc##Impl(T0 A0, T1 A1, T2 A2, T3 A3, T4 A4, T5 A5)

#define LRU_CACHED7(TRet, TFunc, T0, A0, T1, A1, T2, A2, T3, A3, T4, A4,    \
                    T5, A5, T6, A6)                                         \
 TRet TFunc(T0 A0, T1 A1, T2 A2, T3 A3, T4 A4, T5 A5, T6 A6)                \
 {                                                                          \
  _LRU_CACHED_IMPL(TRet, TFunc, A0, A1, A2, A3, A4, A5, A6)                 \
 }                                                                          \
 TRet TFunc##Impl(T0 A0, T1 A1, T2 A2, T3 A3, T4 A4, T5 A5, T6 A6)

#define LRU_CACHED8(TRet, TFunc, T0, A0, T1, A1, T2, A2, T3, A3, T4, A4,    \
                    T5, A5, T6, A6, T7, A7)                                 \
 TRet TFunc(T0 A0, T1 A1, T2 A2, T3 A3, T4 A4, T5 A5, T6 A6, T7 A7)         \
 {                                                                          \
  _LRU_CACHED_IMPL(TRet, TFunc, A0, A1, A2, A3, A4, A5, A6, A7)             \
 }                                                                          \
 TRet TFunc##Impl(T0 A0, T1 A1, T2 A2, T3 A3, T4 A4, T5 A5, T6 A6, T7 A7)

#define LRU_CACHED9(TRet, TFunc, T0, A0, T1, A1, T2, A2, T3, A3, T4, A4,    \
                    T5, A5, T6, A6, T7, A7, T8, A8)                         \
 TRet TFunc(T0 A0, T1 A1, T2 A2, T3 A3, T4 A4, T5 A5, T6 A6, T7 A7, T8 A8)  \
 {                                                                          \
  _LRU_CACHED_IMPL(TRet, TFunc, A0, A1, A2, A3, A4, A5, A6, A7, A8)         \
 }                                                                          \
 TRet TFunc##Impl(T0 A0, T1 A1, T2 A2, T3 A3, T4 A4, T5 A5, T6 A6, T7 A7,   \
                  T8 A8)

#define LRU_CACHED10(TRet, TFunc, T0, A0, T1, A1, T2, A2, T3, A3, T4, A4,   \
                     T5, A5, T6, A6, T7, A7, T8, A8, T9, A9)                \
 TRet TFunc(T0 A0, T1 A1, T2 A2, T3 A3, T4 A4, T5 A5, T6 A6, T7 A7, T8 A8,  \
            T9, A9)                                                         \
 {                                                                          \
  _LRU_CACHED_IMPL(TRet, TFunc, A0, A1, A2, A3, A4, A5, A6, A7, A8, A9)     \
 }                                                                          \
 TRet TFunc##Impl(T0 A0, T1 A1, T2 A2, T3 A3, T4 A4, T5 A5, T6 A6, T7 A7,   \
                  T8 A8, T9 A9)

#endif
