// Copyright 2019-2022 Lawrence Livermore National Security, LLC and other YGM
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#undef NDEBUG

#include <filesystem>
#include <ygm/comm.hpp>
//#include <ygm/io/csv_parser.hpp>
#include <ygm/io/arrow_parquet_parser.hpp>

int main(int argc, char** argv) {
  ygm::comm world(&argc, &argv);

  // assuming the build directory is inside the YGM root directory 
  const std::string dir_name = "data/parquet_files/"; 

  //
  // Test number of lines in files
  {
  // arrow_parquet_parser assumes files have identical scehma
  ygm::io::arrow_parquet_parser parquetp(world, {dir_name});

  // count total number of rows in files   
  size_t local_count = 0;
  
  parquetp.for_all(
    [&local_count](auto& stream_reader, const auto& field_count) {
      stream_reader.SkipColumns(field_count);
      stream_reader.EndRow();
      local_count++; 
    }
  ); 

  world.barrier();
  auto row_count = world.all_reduce_sum(local_count);
  ASSERT_RELEASE(row_count == 12);

  }

  //
  // Test table entries
  {
  // arrow_parquet_parser assumes files have identical scehma
  ygm::io::arrow_parquet_parser parquetp(world, {dir_name});

  // read fields in each row 
  struct columns {
    std::string string_field;
    char char_array_field[4];
    uint64_t uint64_t_field;
    double double_field;
    bool boolean_field;    
  };   

  std::vector<columns> rows;

  parquetp.for_all( 
    [&rows](auto& stream_reader, const auto& field_count) {
      using columns_t = decltype(rows)::value_type;
      columns_t columns_obj;
      stream_reader >> columns_obj.string_field;
      stream_reader >> columns_obj.char_array_field;
      stream_reader >> columns_obj.uint64_t_field;
      stream_reader >> columns_obj.double_field;
      stream_reader >> columns_obj.boolean_field;      
      stream_reader.EndRow();       
      rows.emplace_back(columns_obj); 
    }
  ); 

  world.barrier();
  auto row_count = world.all_reduce_sum(rows.size());   
  ASSERT_RELEASE(row_count == 12);

  if (world.rank() == 0) { 
    auto& obj_ref = rows[1];  
    ASSERT_RELEASE(obj_ref.string_field == "Hennessey Venom F5");
    ASSERT_RELEASE(std::string(obj_ref.char_array_field) == "USA");
    ASSERT_RELEASE(obj_ref.uint64_t_field == 311); 
    ASSERT_RELEASE(obj_ref.double_field == 2.4);
    ASSERT_RELEASE(obj_ref.boolean_field == false); 
  }  

  }

  return 0;
}
