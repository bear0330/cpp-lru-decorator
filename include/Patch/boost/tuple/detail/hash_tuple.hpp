//-----------------------------------------------------------------------------
// boost tuple/detail/hash_tuple.hpp header file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
// http://stackoverflow.com/questions/1250599/how-to-unordered-settupleint-int
// http://lists.boost.org/boost-users/2008/06/37643.php
// Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_HASH_TUPLE_FUNCTION_HPP
#define BOOST_HASH_TUPLE_FUNCTION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include <boost/functional/hash.hpp>
#include <boost/fusion/algorithm/iteration/fold.hpp>
#include <boost/fusion/adapted/boost_tuple.hpp>
#include <boost/tuple/tuple.hpp>

namespace stlex
{
   struct tuple_fusion_hash
   {
      typedef size_t result_type;

      template <typename T>
#if BOOST_VERSION >= 104300
      //NOTE: order changed in Boost 1.43
      std::size_t operator()(std::size_t nSeed, const T& crArg) const
#else
      std::size_t operator()(const T& crArg, std::size_t nSeed) const
#endif
      {
         boost::hash_combine(nSeed, crArg);
         return nSeed;
      }
   };


   struct tuple_hash
   {
      template <typename Tuple>
      std::size_t operator()(const Tuple& cr) const
      {
         return boost::fusion::fold(cr, 0, tuple_fusion_hash());
      }
   };

}   //end namespace stlex


namespace boost
{
   //----------------------------------------------------------------------------
   // template struct tuple_hash
   //----------------------------------------------------------------------------
   // Description: hash function for tuples
   // Note       : must be declared in namespace boost due to ADL
   //----------------------------------------------------------------------------
   /*
   template <class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
   std::size_t hash_value(const boost::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9>& cr)
   {
      const stlex::tuple_hash hsh;
      return hsh(cr);
   }
   */

   // http://svn.boost.org/svn/boost/trunk/boost/functional/hash/extensions.hpp
   namespace hash_detail {
       template <std::size_t I, typename T>
       inline typename boost::enable_if_c<(I == ::boost::tuples::length<T>::value),
               void>::type
           hash_combine_tuple(std::size_t&, T const&)
       {
       }

       template <std::size_t I, typename T>
       inline typename boost::enable_if_c<(I < ::boost::tuples::length<T>::value),
               void>::type
           hash_combine_tuple(std::size_t& seed, T const& v)
       {
           boost::hash_combine(seed, boost::get<I>(v));
           boost::hash_detail::hash_combine_tuple<I + 1>(seed, v);
       }

       template <typename T>
       inline std::size_t hash_tuple(T const& v)
       {
           std::size_t seed = 0;
           boost::hash_detail::hash_combine_tuple<0>(seed, v);
           return seed;
       }
   }

   inline std::size_t hash_value(boost::tuple<> const& v)
   {
       return boost::hash_detail::hash_tuple(v);
   }

#   define BOOST_HASH_TUPLE_F(z, n, _)                                      \
   template<                                                               \
       BOOST_PP_ENUM_PARAMS_Z(z, n, typename A)                            \
   >                                                                       \
   inline std::size_t hash_value(boost::tuple<                             \
       BOOST_PP_ENUM_PARAMS_Z(z, n, A)                                     \
   > const& v)                                                             \
   {                                                                       \
       return boost::hash_detail::hash_tuple(v);                           \
   }

   BOOST_PP_REPEAT_FROM_TO(1, 11, BOOST_HASH_TUPLE_F, _)
#   undef BOOST_HASH_TUPLE_F

   // GUNC
#ifdef __GNUG__
   namespace tuples {
       inline std::size_t hash_value(boost::tuple<> const& v)
       {
           return boost::hash_detail::hash_tuple(v);
       }

    #   define BOOST_HASH_TUPLE_F(z, n, _)                                      \
       template<                                                               \
           BOOST_PP_ENUM_PARAMS_Z(z, n, typename A)                            \
       >                                                                       \
       inline std::size_t hash_value(boost::tuple<                             \
           BOOST_PP_ENUM_PARAMS_Z(z, n, A)                                     \
       > const& v)                                                             \
       {                                                                       \
           return boost::hash_detail::hash_tuple(v);                           \
       }

       BOOST_PP_REPEAT_FROM_TO(1, 11, BOOST_HASH_TUPLE_F, _)
    #   undef BOOST_HASH_TUPLE_F
   }
#endif

}

#endif
