// Copyright 2019-2021 Lawrence Livermore National Security, LLC and other YGM
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <map>
#include <fstream>
#include <ygm/comm.hpp>
#include <ygm/detail/ygm_ptr.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/utility.hpp>
#include <ygm/container/map.hpp>
#include <ygm/container/maptrix.hpp>

namespace ygm::container::detail::algorithms {

  //Create times op.

  //template <typename Key, typename Value, typename OpPlus, typename OpMultiply>
  template <typename Key, typename Value>
  ygm::container::map<Key, Value> spmv(
    ygm::container::maptrix<Key, Value> &A, ygm::container::map<Key, Value> &x) {
    //ygm::container::maptrix<Key, Value> &A, ygm::container::map<Key, Value> &x,
    //    OpPlus plus_op=std::plus<Value>(), OpMultiply times_op=std::times) {
    
    using key_type    = Key;
    using value_type  = Value;
    using map_type    = ygm::container::map<key_type, value_type>;

    auto A_ptr = A.get_ygm_ptr();
    auto A_comm = A_ptr->comm();

    map_type y(A_comm);
    auto y_ptr = y.get_ygm_ptr();

    auto kv_lambda = [A_ptr, y_ptr](auto kv_pair) {

      auto &mptrx_comm = A_ptr->comm();
      int rank         = mptrx_comm.rank();
      
      auto col        = kv_pair.first;
      auto col_value  = kv_pair.second;
      
      auto csc_visit_lambda = [](
        auto col, auto row, 
        auto A_value, auto x_value, auto y_ptr) {

        auto element_wise = A_value * x_value; 
          
        auto append_lambda = [](auto &rv_pair, const auto &update_val) {
          auto row_id = rv_pair.first;
          auto value  = rv_pair.second;
          auto append_val = value + update_val;
          rv_pair.second = rv_pair.second + update_val;
        };

        y_ptr->async_insert_if_missing_else_visit(row, element_wise, append_lambda);
      };
      
      A_ptr->async_visit_col_const(col, csc_visit_lambda, col_value, y_ptr);
    };

    x.for_all(kv_lambda);
    A_comm.barrier();

    return y;
  }

} // namespace ygm::container::detail::algorithms
