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
/*!\file    ByteOrderConversion.cpp
 *
 * \author  Georg Heppner <heppner@fzi.de>
 * \date    2014-02-07
 *
 *
 */
//----------------------------------------------------------------------

#include <schunk_svh_library/serial/ByteOrderConversion.h>


namespace driver_svh {

std::ostream& operator<<(std::ostream& o, const ArrayBuilder& ab)
{
  for (size_t i = 0; i < ab.array.size(); i++)
  {
    o << "0x" << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ab.array[i])
      << " ";
  }
  // Reset Output stream to decimal output .. otherwise it may confuse people
  std::cout << std::dec;
  return o;
}

template <>
size_t toLittleEndian<float>(const float& data, std::vector<uint8_t>& array, size_t& write_pos)
{
  //! As Bit Shifts for floats are dissallowed they have to be given as reinterpreted value to
  //! currectly shift each byte individually
  return toLittleEndian(*(reinterpret_cast<const uint32_t*>(&data)), array, write_pos);
}

template <>
size_t toLittleEndian<double>(const double& data, std::vector<uint8_t>& array, size_t& write_pos)
{
  //! As Bit Shifts for doubles are dissallowed they have to be given as reinterpreted value to
  //! currectly shift each byte individually
  return toLittleEndian(*(reinterpret_cast<const uint64_t*>(&data)), array, write_pos);
}

template <>
size_t fromLittleEndian<float>(float& data, std::vector<uint8_t>& array, size_t& read_pos)
{
  //! As Bit Shifts for floats are dissallowed they have to be given as reinterpreted value to
  //! currectly shift each byte individually
  return fromLittleEndian(*(reinterpret_cast<uint32_t*>(&data)), array, read_pos);
}

template <>
size_t fromLittleEndian<double>(double& data, std::vector<uint8_t>& array, size_t& read_pos)
{
  //! As Bit Shifts for doubles are dissallowed they have to be given as reinterpreted value to
  //! currectly shift each byte individually
  return fromLittleEndian(*(reinterpret_cast<uint64_t*>(&data)), array, read_pos);
}

void ArrayBuilder::reset(size_t array_size)
{
  array.clear();
  write_pos = 0;
  read_pos  = 0;
  array.resize(array_size, 0);
}


} // namespace driver_svh
