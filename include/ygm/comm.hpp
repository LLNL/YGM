// Copyright 2019-2021 Lawrence Livermore National Security, LLC and other YGM
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <deque>
#include <functional>
#include <memory>
#include <vector>

#include <ygm/detail/comm_environment.hpp>
#include <ygm/detail/comm_router.hpp>
#include <ygm/detail/comm_stats.hpp>
#include <ygm/detail/lambda_map.hpp>
#include <ygm/detail/layout.hpp>
#include <ygm/detail/meta/functional.hpp>
#include <ygm/detail/mpi.hpp>
#include <ygm/detail/ygm_cereal_archive.hpp>
#include <ygm/detail/ygm_ptr.hpp>

namespace ygm {

namespace detail {
class interrupt_mask;
class comm_stats;
class layout;
class comm_router;
}  // namespace detail

class comm {
 private:
  class mpi_irecv_request;
  class mpi_isend_request;
  class header_t;
  friend class detail::interrupt_mask;
  friend class detail::comm_stats;

 public:
  comm(int *argc, char ***argv);

  // TODO:  Add way to detect if this MPI_Comm is already open. E.g., static
  // map<MPI_Comm, impl*>
  comm(MPI_Comm comm);

  ~comm();

  /**
   * @brief Prints a welcome message with configuration details.
   *
   */
  void welcome(std::ostream &os = std::cout);

  void stats_reset();
  void stats_print(const std::string &name = "", std::ostream &os = std::cout);

  //
  //  Asynchronous rpc interfaces.   Can be called inside OpenMP loop
  //

  template <typename AsyncFunction, typename... SendArgs>
  void async(int dest, AsyncFunction fn, const SendArgs &...args);

  template <typename AsyncFunction, typename... SendArgs>
  void async_bcast(AsyncFunction fn, const SendArgs &...args);

  template <typename AsyncFunction, typename... SendArgs>
  void async_mcast(const std::vector<int> &dests, AsyncFunction fn,
                   const SendArgs &...args);

  //
  // Collective operations across all ranks.  Cannot be called inside OpenMP
  // region.

  /**
   * @brief Control Flow Barrier
   * Only blocks the control flow until all processes in the communicator have
   * called it. See:  MPI_Barrier()
   */
  void cf_barrier();

  /**
   * @brief Full communicator barrier
   *
   */
  void barrier();

  template <typename T>
  ygm_ptr<T> make_ygm_ptr(T &t);

  /**
   * @brief Registers a callback that will be executed prior to the barrier
   * completion
   *
   * @param fn callback function
   */
  void register_pre_barrier_callback(const std::function<void()> &fn);

  template <typename T>
  T all_reduce_sum(const T &t) const;

  template <typename T>
  T all_reduce_min(const T &t) const;

  template <typename T>
  T all_reduce_max(const T &t) const;

  template <typename T, typename MergeFunction>
  inline T all_reduce(const T &t, MergeFunction merge) const;

  //
  //  Communicator information
  //
  int size() const;
  int rank() const;

  MPI_Comm get_mpi_comm() const;

  const detail::layout &layout() const;

  const detail::comm_router &router() const;

  bool rank0() const { return rank() == 0; }

  template <typename T>
  void mpi_send(const T &data, int dest, int tag, MPI_Comm comm) const;

  template <typename T>
  T mpi_recv(int source, int tag, MPI_Comm comm) const;

  template <typename T>
  T mpi_bcast(const T &to_bcast, int root, MPI_Comm comm) const;

  std::ostream &cout0() const;
  std::ostream &cerr0() const;
  std::ostream &cout() const;
  std::ostream &cerr() const;

  template <typename... Args>
  void cout(Args &&...args) const;

  template <typename... Args>
  void cerr(Args &&...args) const;

  template <typename... Args>
  void cout0(Args &&...args) const;

  template <typename... Args>
  void cerr0(Args &&...args) const;

  // Private member functions
 private:
  void comm_setup(MPI_Comm comm);

  size_t pack_header(std::vector<std::byte> &packed, const int dest,
                     size_t size);

  std::pair<uint64_t, uint64_t> barrier_reduce_counts();

  void flush_send_buffer(int dest);

  void check_if_production_halt_required();

  void flush_all_local_and_process_incoming();

  void flush_to_capacity();

  void post_new_irecv(std::shared_ptr<std::byte[]> &recv_buffer);

  template <typename Lambda, typename... PackArgs>
  size_t pack_lambda(std::vector<std::byte> &packed, Lambda l,
                     const PackArgs &...args);

  template <typename Lambda, typename... PackArgs>
  void pack_lambda_broadcast(Lambda l, const PackArgs &...args);

  template <typename Lambda, typename RemoteLogicLambda, typename... PackArgs>
  size_t pack_lambda_generic(std::vector<std::byte> &packed, Lambda l,
                             RemoteLogicLambda rll, const PackArgs &...args);

  void queue_message_bytes(const std::vector<std::byte> &packed,
                           const int                     dest);

  void handle_next_receive(MPI_Status                   status,
                           std::shared_ptr<std::byte[]> buffer);

  bool process_receive_queue();

  template <typename... Args>
  std::string outstr(Args &&...args) const;

  template <typename... Args>
  std::string outstr0(Args &&...args) const;

  comm() = delete;

  comm(const comm &c) = delete;

  // Private member variables
 private:
  std::shared_ptr<detail::mpi_init_finalize> pimpl_if;

  MPI_Comm m_comm_async;
  MPI_Comm m_comm_barrier;
  MPI_Comm m_comm_other;

  std::vector<std::vector<std::byte>> m_vec_send_buffers;
  size_t                              m_send_buffer_bytes = 0;
  std::deque<int>                     m_send_dest_queue;

  std::deque<mpi_irecv_request>                        m_recv_queue;
  std::deque<mpi_isend_request>                        m_send_queue;
  std::vector<std::shared_ptr<std::vector<std::byte>>> m_free_send_buffers;

  size_t m_pending_isend_bytes = 0;

  std::deque<std::function<void()>> m_pre_barrier_callbacks;

  bool m_enable_interrupts = true;

  uint64_t m_recv_count = 0;
  uint64_t m_send_count = 0;

  bool m_in_process_receive_queue = false;

  detail::comm_stats             stats;
  const detail::comm_environment config;
  const detail::layout           m_layout;
  detail::comm_router            m_router;

  detail::lambda_map<void (*)(comm *, cereal::YGMInputArchive *), uint16_t>
      m_lambda_map;
};

}  // end namespace ygm

#include <ygm/detail/comm.ipp>
//#include <ygm/detail/comm_impl.hpp>
