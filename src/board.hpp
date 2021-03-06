#pragma once

#include "defs.hpp"

#include <iostream>
#include <vector>

constexpr int CASTLE_KING_WHITE = 1;
constexpr int CASTLE_QUEEN_WHITE = 2;
constexpr int CASTLE_KING_BLACK = 4;
constexpr int CASTLE_QUEEN_BLACK = 8;

struct Piece
{
  int type;
  int color;
};

class Move;

class Board
{
private:
  U64 _pieces[BOTH][EMPTY];
  U64 _occupancies[BOTH + 1];
  int _to_move;
  int _castling_rights;
  int _en_passant_square;
  int _half_move_clock;
  int _full_move_number;

  bool _white_on_bottom;
  Piece _square[N_SQUARES];

  void updateOccupancies();
  void updateBBFromSquares();

public:
  Board();
  Board(const Board &board);

  void clear();
  void setStartingPosition();
  void setFromFen(std::string piece_placements,
                  std::string active_color,
                  std::string castling_rights,
                  std::string en_passant,
                  std::string halfmove_clock,
                  std::string fullmove_number);
  int switchSideToMove();
  bool makeMove(Move move);
  bool makeMoveFromUCI(std::string move);
  std::vector<std::string> getLegalMovesUCI();
  bool rotate();
  void print(bool ascii = false);

  bool isSquareAttacked(const int sq, const int attacker_side) const;
  int getSideToMove() const;
  int getCastlingRights() const;
  int getEnPassantSquare() const;
  int getHalfMoveClock() const;
  int getFullMoveNumber() const;
  U64 getOccupiedSquares() const;
  std::string getFen() const;
  std::vector<Move> getPseudoLegalMoves() const;
};
