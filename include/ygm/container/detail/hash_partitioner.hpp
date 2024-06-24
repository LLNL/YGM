// Copyright 2019-2021 Lawrence Livermore National Security, LLC and other YGM
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#pragma once
// #include <bit>
#include <functional>

namespace ygm::container::detail {

template <typename Key>
struct old_hash_partitioner {
  std::pair<size_t, size_t> operator()(const Key &k, size_t nranks,
                                       size_t nbanks) const {
    size_t hash = std::hash<Key>{}(k);
    size_t rank = hash % nranks;
    size_t bank = (hash / nranks) % nbanks;
    return std::make_pair(rank, bank);
  }
};

template <typename Hash>
struct hash_partitioner {
  hash_partitioner(ygm::comm &comm, Hash hash)
      : m_comm_size(comm.size()), m_hash(hash) {}
  template <typename Key>
  int owner(const Key &key) {
    return (m_hash(key) * 2654435769L >> 32) % m_comm_size;
  }

 private:
  int  m_comm_size;
  Hash m_hash;
};

}  // namespace ygm::container::detail
