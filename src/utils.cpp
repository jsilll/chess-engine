#include "utils.hpp"

#include "tables.hpp"

#include <string>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <regex>
#include <vector>

int Utils::bitScan(U64 bb)
{
  if (bb)
  {
    return Utils::bitCount((bb & -bb) - 1);
  }
  return -1;
}

int Utils::bitScanForward(U64 bb)
{
  static const int index64[64] = {
      0, 47, 1, 56, 48, 27, 2, 60,
      57, 49, 41, 37, 28, 16, 3, 61,
      54, 58, 35, 52, 50, 42, 21, 44,
      38, 32, 29, 23, 17, 11, 4, 62,
      46, 55, 26, 59, 40, 36, 15, 53,
      34, 51, 20, 43, 31, 22, 10, 45,
      25, 39, 14, 33, 19, 30, 9, 24,
      13, 18, 8, 12, 7, 6, 5, 63};

  static const U64 debruijn64 = 0x03f79d71b4cb0a89;
  return index64[((bb ^ (bb - 1)) * debruijn64) >> 58];
}

U64 Utils::setOccupancy(int index, int bits_in_mask, U64 attack_mask)
{
  U64 occupancy = ZERO;
  for (int bit = 0; bit < bits_in_mask; bit++)
  {
    int lsb_sq = Utils::bitScan(attack_mask);
    Utils::popBit(attack_mask, lsb_sq);
    if (index & (1 << bit))

    {
      occupancy |= Tables::SQUARE_BB[lsb_sq];
    }
  }
  return occupancy;
}

void Utils::printBB(U64 bb)
{
  for (int i = 7; i >= 0; i--)
  {
    std::cout << i + 1 << "  ";
    for (int n = 0; n < 8; n++)
    {
      std::cout << ((bb >> getSquare(i, n)) & ONE) << " ";
    }
    std::cout << "\n";
  }
  std::cout << "\n   a b c d e f g h\n\n";
  printf("bitboard %llud\n", bb);
}

std::vector<std::string> Utils::tokenizeString(const std::string &text)
{
  std::vector<std::string> tokens{};
  std::istringstream iss(text);
  copy(std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>(), std::back_inserter(tokens));
  return tokens;
}
