// Copyright 2019-2021 Lawrence Livermore National Security, LLC and other YGM
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <ygm/io/detail/csv.hpp>
#include <ygm/io/line_parser.hpp>

namespace ygm::io {

class csv_parser {
 public:
  template <typename... Args>
  csv_parser(Args&&... args)
      : m_lp(std::forward<Args>(args)...), m_has_headers(false) {}

  /**
   * @brief Executes a user function for every CSV record in a set of files.
   *
   * @tparam Function
   * @param fn User function to execute
   */
  template <typename Function>
  void for_all(Function fn) {
    using namespace ygm::io::detail;

    std::map<std::string, int>* header_map_ptr;
    bool                        skip_first;
    auto handle_line_lambda = [fn, &header_map_ptr](const std::string& line) {
      auto vfields = parse_csv_line(line, header_map_ptr);
      // auto stypes    = convert_type_string(vfields);
      // todo, detect if types are inconsistent between records
      if (vfields.size() > 0) {
        fn(vfields);
      }
    };

    if (m_has_headers) {
      header_map_ptr = &m_header_map;
      m_lp.for_all(handle_line_lambda, true);
    } else {
      header_map_ptr = nullptr;
      m_lp.for_all(handle_line_lambda);
    }

    m_lp.for_all([fn, header_map_ptr](const std::string& line) {});
  }

  /**
   * @brief Read the header of a CSV file
   */
  void read_headers() {
    using namespace ygm::io::detail;
    auto header_line = m_lp.read_first();
    m_header_map     = parse_csv_headers(header_line);
    m_has_headers    = true;
  }

 private:
  line_parser m_lp;

  std::map<std::string, int> m_header_map;
  bool                       m_has_headers;
};  // namespace ygm::io
}  // namespace ygm::io
