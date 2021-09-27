// Copyright 2019-2021 Lawrence Livermore National Security, LLC and other YGM
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#undef NDEBUG

#include <string>
#include <ygm/comm.hpp>
#include <ygm/container/disjoint_set.hpp>

int main(int argc, char** argv) {
  ygm::comm world(&argc, &argv);

  //
  // Test async_make_set from a single rank
  {
    ygm::container::disjoint_set<std::string> dset(world);

    if (world.rank0()) {
      dset.async_make_set("cat");
      dset.async_make_set("dog");
    }
    ASSERT_RELEASE(dset.num_sets() == 2);
  }

  //
  // Test async_union from single rank
  {
    ygm::container::disjoint_set<std::string> dset(world);

    if (world.rank0()) {
      dset.async_make_set("cat");
      dset.async_make_set("dog");
      dset.async_make_set("car");
    }

    world.barrier();

    if (world.rank0()) {
      dset.async_union("cat", "dog");
    }

    std::vector<std::string> to_find = {"cat", "dog", "car"};

    auto reps = dset.all_find(to_find);
    ASSERT_RELEASE(reps["cat"] == reps["dog"]);
    ASSERT_RELEASE(reps["cat"] != reps["car"]);
  }

  //
  // Test async_union from all ranks
  {
    ygm::container::disjoint_set<std::string> dset(world);

    if (world.rank0()) {
      dset.async_make_set("cat");
      dset.async_make_set("dog");
      dset.async_make_set("car");
    }

    world.barrier();

    dset.async_union("cat", "dog");

    std::vector<std::string> to_find = {"cat", "dog", "car"};

    auto reps = dset.all_find(to_find);
    ASSERT_RELEASE(reps["cat"] == reps["dog"]);
    ASSERT_RELEASE(reps["cat"] != reps["car"]);
  }

  //
  // Test longer union chains
  {
    ygm::container::disjoint_set<int> dset(world);

    if (world.rank0()) {
      dset.async_make_set(0);
      dset.async_make_set(1);
      dset.async_make_set(2);
      dset.async_make_set(3);
      dset.async_make_set(4);
      dset.async_make_set(5);
    }

    world.barrier();
    ASSERT_RELEASE(dset.num_sets() == 6);

    std::vector<int> to_find = {0, 1, 2, 3, 4, 5};

    dset.async_union(0, 1);
    dset.async_union(1, 2);

    dset.async_union(3, 4);
    dset.async_union(4, 5);

    ASSERT_RELEASE(dset.num_sets() == 2);

    auto reps = dset.all_find(to_find);
    ASSERT_RELEASE(reps[0] == reps[1]);
    ASSERT_RELEASE(reps[1] == reps[2]);
    ASSERT_RELEASE(reps[2] != reps[3]);
    ASSERT_RELEASE(reps[3] == reps[4]);
    ASSERT_RELEASE(reps[4] == reps[5]);

    dset.async_union(0, 3);
    ASSERT_RELEASE(dset.num_sets() == 1);

    auto reps_final = dset.all_find(to_find);
    ASSERT_RELEASE(reps_final[0] == reps_final[1]);
    ASSERT_RELEASE(reps_final[1] == reps_final[2]);
    ASSERT_RELEASE(reps_final[2] == reps_final[3]);
    ASSERT_RELEASE(reps_final[3] == reps_final[4]);
    ASSERT_RELEASE(reps_final[4] == reps_final[5]);
  }

  //
  // Test longer union chains with different union order
  {
    ygm::container::disjoint_set<int> dset(world);

    if (world.rank0()) {
      dset.async_make_set(0);
      dset.async_make_set(1);
      dset.async_make_set(2);
      dset.async_make_set(3);
      dset.async_make_set(4);
      dset.async_make_set(5);
    }

    world.barrier();
    ASSERT_RELEASE(dset.num_sets() == 6);

    std::vector<int> to_find = {0, 1, 2, 3, 4, 5};

    dset.async_union(0, 2);
    dset.async_union(1, 2);

    dset.async_union(4, 5);
    dset.async_union(3, 5);

    ASSERT_RELEASE(dset.num_sets() == 2);

    auto reps = dset.all_find(to_find);
    ASSERT_RELEASE(reps[0] == reps[1]);
    ASSERT_RELEASE(reps[1] == reps[2]);
    ASSERT_RELEASE(reps[2] != reps[3]);
    ASSERT_RELEASE(reps[3] == reps[4]);
    ASSERT_RELEASE(reps[4] == reps[5]);

    dset.async_union(0, 3);
    ASSERT_RELEASE(dset.num_sets() == 1);

    dset.all_compress();

    auto reps_final = dset.all_find(to_find);
    ASSERT_RELEASE(reps_final[0] == reps_final[1]);
    ASSERT_RELEASE(reps_final[1] == reps_final[2]);
    ASSERT_RELEASE(reps_final[2] == reps_final[3]);
    ASSERT_RELEASE(reps_final[3] == reps_final[4]);
    ASSERT_RELEASE(reps_final[4] == reps_final[5]);
  }
}
