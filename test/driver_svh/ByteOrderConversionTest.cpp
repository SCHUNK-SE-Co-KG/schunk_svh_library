////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2024 SCHUNK SE & Co. KG, Lauffen/Neckar Germany
// Copyright 2022 FZI Forschungszentrum Informatik, Karlsruhe, Germany
//
// This file is part of the Schunk SVH Library.
//
// The Schunk SVH Library is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// The Schunk SVH Library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
// Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// the Schunk SVH Library. If not, see <https://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------
/*!\file
 *
 * \author  Lars Pfotzer
 * \date    2014-02-03
 *
 */
//----------------------------------------------------------------------
#include <schunk_svh_library/serial/ByteOrderConversion.h>

#include <boost/test/unit_test.hpp>

using driver_svh::ArrayBuilder;

BOOST_AUTO_TEST_SUITE(ts_ByteOrderConversion)


BOOST_AUTO_TEST_CASE(ConvertTest)
{
  float test_float     = 15.08;
  int test_int         = 1508;
  u_int8_t test_uint   = 128;
  u_int16_t test_uint2 = 128;
  ArrayBuilder ab;
  ab << test_int << test_float << test_uint << test_uint2;

  // std::cout << "ArrayBuilder read the values " << test_int << " , " << test_float << " , " <<
  // (int)test_uint << " , is currently at POS(r/w): "<<(int)ab.read_pos<<"/"<<(int)ab.write_pos<< "
  // And contains the " << (int)ab.array.size() << " Byte long Byte array: " << ab << std::endl;

  float test_float_out     = 0.0;
  int test_int_out         = 0;
  u_int8_t test_uint_out   = 0;
  u_int16_t test_uint2_out = 2;
  int test_to_mouch        = 0;

  ab >> test_int_out >> test_float_out >> test_uint_out >> test_uint2_out >> test_to_mouch;

  // std::cout << "ArrayBuilder reconstructed the values: "<< test_int_out << " , " <<
  // test_float_out << " , " << (int)test_uint_out << " , is currently at POS(r/w):
  // "<<(int)ab.read_pos<<"/"<<(int)ab.write_pos<< " And contains the " << (int)ab.array.size() << "
  // Byte long Byte array: " << ab << std::endl; std::cout << "This value should contain 0: "<<
  // test_to_mouch << std::endl;

  BOOST_CHECK_EQUAL(test_float, test_float_out);
  BOOST_CHECK_EQUAL(test_int, test_int_out);
  BOOST_CHECK_EQUAL(test_uint, test_uint_out);
  BOOST_CHECK_EQUAL(test_uint2, test_uint2_out);
  BOOST_CHECK_EQUAL(test_to_mouch, 0);
}

BOOST_AUTO_TEST_CASE(ConvertVectorsTest)
{
  ArrayBuilder ab;
  std::vector<u_int8_t> test_uint8_vector;
  std::vector<u_int16_t> test_uint16_vector;
  std::vector<u_int8_t> test_uint8_vector_out(4, 0);
  std::vector<u_int16_t> test_uint16_vector_out(4, 0);

  // Test if the Vectors Work with the ArrayBuilder
  test_uint8_vector.push_back(1);
  test_uint8_vector.push_back(2);
  test_uint8_vector.push_back(3);
  test_uint8_vector.push_back(4);
  ab << test_uint8_vector;

  // std::cout << "Put a vector with the uint8_t type and the data 1,2,3,4 into the array builder,
  // RAW Data: "<< ab << std::endl;

  test_uint16_vector.push_back(5);
  test_uint16_vector.push_back(6);
  test_uint16_vector.push_back(7);
  test_uint16_vector.push_back(8);
  ab << test_uint16_vector;

  // std::cout << "Added a vector with the uint16_t type and the data 5,6,7,8 into the array
  // builder, RAW Data: "<< ab << std::endl;

  // Read out the arrays aggain
  ab >> test_uint8_vector_out;
  ab >> test_uint16_vector_out;


  //  std::cout << "Test_uint_16_vector was reconstructed with the elements: ";
  //  for (std::vector<u_int16_t>::const_iterator it = test_uint16_vector_out.begin() ; it !=
  //  test_uint16_vector_out.end(); ++it)
  //  {
  //    std::cout << (int)*it << ", ";
  //  }
  //  std::cout << std::endl;

  //  std::cout << "Test_uint_8_vector was reconstructed with the elements: ";
  //  for (std::vector<u_int8_t>::const_iterator it = test_uint8_vector_out.begin() ; it !=
  //  test_uint8_vector_out.end(); ++it)
  //  {
  //    std::cout << (int)*it << ", ";
  //  }
  //  std::cout << std::endl;


  BOOST_CHECK_EQUAL_COLLECTIONS(test_uint8_vector.begin(),
                                test_uint8_vector.end(),
                                test_uint8_vector_out.begin(),
                                test_uint8_vector_out.end());
  BOOST_CHECK_EQUAL_COLLECTIONS(test_uint16_vector.begin(),
                                test_uint16_vector.end(),
                                test_uint16_vector_out.begin(),
                                test_uint16_vector_out.end());
}

BOOST_AUTO_TEST_CASE(ReadBackTest)
{
  ArrayBuilder ab;

  u_int32_t size      = 12;
  u_int32_t size_peek = 0;
  u_int32_t size_out  = 0;

  ab << size;
  size_peek = ab.readBack<u_int32_t>();
  ab >> size_out;

  // std::cout << "Size: "<<(int)size << " Size_peek "<< size_peek << " size_out" << size_out <<
  // std::endl;
  BOOST_CHECK_EQUAL(size, size_out);
  BOOST_CHECK_EQUAL(size, size_peek);

  ab >> size_out;

  // This should return zero as the read pointer is empty.,However,readback should still function as
  // the last element written is still there
  BOOST_CHECK_EQUAL(size_out, 0);
  size_peek = ab.readBack<u_int32_t>();
  BOOST_CHECK_EQUAL(size, size_peek);
}


BOOST_AUTO_TEST_SUITE_END()
