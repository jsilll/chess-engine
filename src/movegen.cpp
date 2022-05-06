#include "movegen.hpp"

#include "utils.hpp"
#include "attacks.hpp"
#include "tables.hpp"
#include "board.hpp"
#include "move.hpp"
#include "movelist.hpp"

MoveList Movegen::generateLegalMoves(const Board &board);
MoveList Movegen::generatePseudoLegalMoves(const Board &board);

inline void generateCastlingMoves(MoveList &move_list, const Board &board, int opponent, int castle_b_sq, int castle_c_sq, int castle_d_sq, int castle_e_sq, int castle_f_sq, int castle_g_sq, int castle_queen_mask, int castle_king_mask);

inline void generatePawnDoublePushes(MoveList &move_list, U64 pawn_double_pushes, int pawn_double_push_offset);
inline void generatePawnSinglePushWithPromotion(MoveList &move_list, U64 pawn_single_pushes, int pawn_single_push_offset);
inline void generatePawnSinglePushNoPromotion(MoveList &move_list, U64 pawn_single_pushes, int pawn_single_push_offset);
inline void generatePawnCapturesWithPromotion(MoveList &move_list, int to_move, U64 to_move_pawns, U64 opponent_occupancies);
inline void generatePawnCapturesNoPromotion(MoveList &move_list, int to_move, U64 to_move_pawns, U64 opponent_occupancies);
inline void generateEnPassantCapture(MoveList &move_list, U64 to_move_pawns, int en_passant_square, int opponent);

inline void generateKnightMoves(MoveList &move_list, U64 to_move_knights, U64 to_move_occupancies, U64 opponent_occupancies);
inline void generateKingMoves(MoveList &move_list, U64 to_move_king, U64 to_move_occupancies, U64 opponent_occupancies);

inline void generateBishopMoves(MoveList &move_list, U64 to_move_bishops, U64 to_move_occupancies, U64 opponent_occupancies, U64 both_occupancies);
inline void generateRookMoves(MoveList &move_list, U64 to_move_rooks, U64 to_move_occupancies, U64 opponent_occupancies, U64 both_occupancies);
inline void generateQueenMoves(MoveList &move_list, U64 to_move_queens, U64 to_move_occupancies, U64 opponent_occupancies, U64 both_occupancies);

MoveList Movegen::generatePseudoLegalMoves(const Board &board)
{
  MoveList move_list;

  int to_move = board.getSideToMove();
  int opponent = board.getOpponent();

  U64 to_move_pawns = board.getPieces(to_move, PAWN);

  U64 to_move_occupancies = board.getOccupancies(to_move);
  U64 opponent_occupancies = board.getOccupancies(opponent);
  U64 both_occupancies = board.getOccupancies(BOTH);

  int castle_b_sq, castle_c_sq, castle_d_sq, castle_e_sq, castle_f_sq, castle_g_sq;
  int castle_king_mask, castle_queen_mask;
  int pawn_double_push_offset, pawn_single_push_offset;
  U64 pawn_double_pushes, pawn_single_pushes;
  if (to_move == WHITE)
  {
    castle_b_sq = B1;
    castle_c_sq = C1;
    castle_d_sq = D1;
    castle_e_sq = E1;
    castle_f_sq = F1;
    castle_g_sq = G1;
    castle_king_mask = CASTLE_KING_WHITE;
    castle_queen_mask = CASTLE_QUEEN_WHITE;

    pawn_double_push_offset = -16;
    pawn_single_push_offset = pawn_double_push_offset / 2;
    pawn_double_pushes = Attacks::maskWhitePawnDoublePushes(to_move_pawns, ~both_occupancies);
    pawn_single_pushes = Attacks::maskWhitePawnSinglePushes(to_move_pawns, ~both_occupancies);
  }
  else
  {
    castle_b_sq = B8;
    castle_c_sq = C8;
    castle_d_sq = D8;
    castle_e_sq = E8;
    castle_f_sq = F8;
    castle_g_sq = G8;
    castle_king_mask = CASTLE_KING_BLACK;
    castle_queen_mask = CASTLE_QUEEN_BLACK;

    pawn_double_push_offset = 16;
    pawn_single_push_offset = pawn_double_push_offset / 2;
    pawn_double_pushes = Attacks::maskBlackPawnDoublePushes(to_move_pawns, ~both_occupancies);
    pawn_single_pushes = Attacks::maskBlackPawnSinglePushes(to_move_pawns, ~both_occupancies);
  }

  generatePawnCapturesWithPromotion(move_list, to_move, to_move_pawns, opponent_occupancies);
  generatePawnCapturesNoPromotion(move_list, to_move, to_move_pawns, opponent_occupancies);
  generateEnPassantCapture(move_list, to_move_pawns, board.getEnPassantSquare(), opponent);
  generatePawnSinglePushWithPromotion(move_list, pawn_single_pushes, pawn_single_push_offset);
  generatePawnDoublePushes(move_list, pawn_double_pushes, pawn_double_push_offset);
  generatePawnSinglePushNoPromotion(move_list, pawn_single_pushes, pawn_single_push_offset);
  generateKnightMoves(move_list, board.getPieces(to_move, KNIGHT), to_move_occupancies, opponent_occupancies);
  generateBishopMoves(move_list, board.getPieces(to_move, BISHOP), to_move_occupancies, opponent_occupancies, both_occupancies);
  generateRookMoves(move_list, board.getPieces(to_move, ROOK), to_move_occupancies, opponent_occupancies, both_occupancies);
  generateQueenMoves(move_list, board.getPieces(to_move, QUEEN), to_move_occupancies, opponent_occupancies, both_occupancies);
  generateCastlingMoves(move_list, board, opponent, castle_b_sq, castle_c_sq, castle_d_sq, castle_e_sq, castle_f_sq, castle_g_sq, castle_queen_mask, castle_king_mask);
  generateKingMoves(move_list, board.getPieces(to_move, KING), to_move_occupancies, opponent_occupancies);

  return move_list;
}

MoveList Movegen::generateLegalMoves(const Board &board)
{
  MoveList legal_moves;
  for (Move const &move : Movegen::generatePseudoLegalMoves(board))
  {
    Board backup = board;
    backup.makeMove(move);
    int king_sq = Utils::bitScanForward(backup.getPieces(board.getSideToMove(), KING));
    int attacker_side = backup.getSideToMove();
    if (!backup.isSquareAttacked(king_sq, attacker_side))
    {
      legal_moves.push_back(move);
    }
  }
  return legal_moves;
}

void generateCastlingMoves(MoveList &move_list, const Board &board, int opponent, int castle_b_sq, int castle_c_sq, int castle_d_sq, int castle_e_sq, int castle_f_sq, int castle_g_sq, int castle_queen_mask, int castle_king_mask)
{
  if (!board.isSquareAttacked(castle_e_sq, opponent))
  {
    if ((board.getCastlingRights() & castle_king_mask) && !Utils::getBit(board.getOccupancies(BOTH), castle_f_sq) && !Utils::getBit(board.getOccupancies(BOTH), castle_g_sq))
    {
      if (!board.isSquareAttacked(castle_f_sq, opponent && !board.isSquareAttacked(castle_g_sq, opponent)))
      {
        move_list.push_back(Move(castle_e_sq, castle_g_sq, KING, 0, false, false, false, true));
      }
    }
    if ((board.getCastlingRights() & castle_queen_mask) && !Utils::getBit(board.getOccupancies(BOTH), castle_d_sq) && !Utils::getBit(board.getOccupancies(BOTH), castle_c_sq) && !Utils::getBit(board.getOccupancies(BOTH), castle_b_sq))
    {
      if (!board.isSquareAttacked(castle_d_sq, opponent) && !board.isSquareAttacked(castle_c_sq, opponent))
      {
        move_list.push_back(Move(castle_e_sq, castle_c_sq, KING, 0, false, false, false, true));
      }
    }
  }
}

void generatePawnDoublePushes(MoveList &move_list, U64 pawn_double_pushes, int pawn_double_push_offset)
{
  while (pawn_double_pushes)
  {
    int to_square = Utils::bitScanForward(pawn_double_pushes);
    int from_square = to_square + pawn_double_push_offset;
    move_list.push_back(Move(from_square, to_square, PAWN, 0, false, true, false, false));
    Utils::popLastBit(pawn_double_pushes);
  }
}

void generatePawnSinglePushWithPromotion(MoveList &move_list, U64 pawn_single_pushes, int pawn_single_push_offset)
{
  U64 pawn_single_pushes_promo = pawn_single_pushes & (Tables::MASK_RANK[0] | Tables::MASK_RANK[7]);
  while (pawn_single_pushes_promo)
  {
    int to_square = Utils::bitScanForward(pawn_single_pushes_promo);
    int from_square = to_square + pawn_single_push_offset;
    move_list.push_back(Move(from_square, to_square, PAWN, KNIGHT, false, false, false, false));
    move_list.push_back(Move(from_square, to_square, PAWN, BISHOP, false, false, false, false));
    move_list.push_back(Move(from_square, to_square, PAWN, ROOK, false, false, false, false));
    move_list.push_back(Move(from_square, to_square, PAWN, QUEEN, false, false, false, false));
    Utils::popLastBit(pawn_single_pushes_promo);
  }
}

void generatePawnSinglePushNoPromotion(MoveList &move_list, U64 pawn_single_pushes, int pawn_single_push_offset)
{
  U64 pawn_single_pushes_no_promo = pawn_single_pushes & Tables::MASK_CLEAR_RANK[0] & Tables::MASK_CLEAR_RANK[7];
  while (pawn_single_pushes_no_promo)
  {
    int to_square = Utils::bitScanForward(pawn_single_pushes_no_promo);
    int from_square = to_square + pawn_single_push_offset;
    move_list.push_back(Move(from_square, to_square, PAWN, 0, false, false, false, false));
    Utils::popLastBit(pawn_single_pushes_no_promo);
  }
}

void generatePawnCapturesWithPromotion(MoveList &move_list, int to_move, U64 to_move_pawns, U64 opponent_occupancies)
{
  U64 pawns_can_capture_with_promo = to_move_pawns & Tables::MASK_RANK[6 - (5 * to_move)];
  while (pawns_can_capture_with_promo)
  {
    int from_square = Utils::bitScanForward(pawns_can_capture_with_promo);
    U64 pawn_captures_promo = Tables::ATTACKS_PAWN[to_move][from_square] & opponent_occupancies;
    while (pawn_captures_promo)
    {
      int to_square = Utils::bitScanForward(pawn_captures_promo);
      move_list.push_back(Move(from_square, to_square, PAWN, KNIGHT, true, false, false, false));
      move_list.push_back(Move(from_square, to_square, PAWN, BISHOP, true, false, false, false));
      move_list.push_back(Move(from_square, to_square, PAWN, ROOK, true, false, false, false));
      move_list.push_back(Move(from_square, to_square, PAWN, QUEEN, true, false, false, false));
      Utils::popLastBit(pawn_captures_promo);
    }
    Utils::popLastBit(pawns_can_capture_with_promo);
  }
}

void generatePawnCapturesNoPromotion(MoveList &move_list, int to_move, U64 to_move_pawns, U64 opponent_occupancies)
{
  U64 pawns_can_capture_no_promo = to_move_pawns & Tables::MASK_CLEAR_RANK[6 - (5 * to_move)];
  while (pawns_can_capture_no_promo)
  {
    int from_square = Utils::bitScanForward(pawns_can_capture_no_promo);
    U64 pawn_captures_no_promo = Tables::ATTACKS_PAWN[to_move][from_square] & opponent_occupancies;
    while (pawn_captures_no_promo)
    {
      int to_square = Utils::bitScanForward(pawn_captures_no_promo);
      move_list.push_back(Move(from_square, to_square, PAWN, 0, true, false, false, false));
      Utils::popLastBit(pawn_captures_no_promo);
    }
    Utils::popLastBit(pawns_can_capture_no_promo);
  }
}

void generateEnPassantCapture(MoveList &move_list, U64 to_move_pawns, int en_passant_square, int opponent)
{
  if (en_passant_square != -1)
  {
    U64 pawns_can_en_passant = Tables::ATTACKS_PAWN[opponent][en_passant_square] & to_move_pawns;
    while (pawns_can_en_passant)
    {
      int from_square = Utils::bitScanForward(pawns_can_en_passant);
      move_list.push_back(Move(from_square, en_passant_square, PAWN, 0, true, false, true, false));
      Utils::popLastBit(pawns_can_en_passant);
    }
  }
}

void generateKnightMoves(MoveList &move_list, U64 to_move_knights, U64 to_move_occupancies, U64 opponent_occupancies)
{
  while (to_move_knights)
  {
    int from_square = Utils::bitScanForward(to_move_knights);
    U64 moves = Tables::ATTACKS_KNIGHT[from_square] & ~to_move_occupancies;
    while (moves)
    {
      int to_square = Utils::bitScanForward(moves);
      move_list.push_back(Move(from_square, to_square, KNIGHT, 0, Utils::getBit(opponent_occupancies, to_square), false, false, false));
      Utils::popLastBit(moves);
    }
    Utils::popLastBit(to_move_knights);
  }
}

void generateBishopMoves(MoveList &move_list, U64 to_move_bishops, U64 to_move_occupancies, U64 opponent_occupancies, U64 both_occupancies)
{
  while (to_move_bishops)
  {
    int from_square = Utils::bitScanForward(to_move_bishops);
    U64 moves = Tables::getBishopAttacks(from_square, both_occupancies) & ~to_move_occupancies;
    while (moves)
    {
      int to_square = Utils::bitScanForward(moves);
      move_list.push_back(Move(from_square, to_square, BISHOP, 0, Utils::getBit(opponent_occupancies, to_square), false, false, false));
      Utils::popLastBit(moves);
    }
    Utils::popLastBit(to_move_bishops);
  }
}

void generateRookMoves(MoveList &move_list, U64 to_move_rooks, U64 to_move_occupancies, U64 opponent_occupancies, U64 both_occupancies)
{
  while (to_move_rooks)
  {
    int from_square = Utils::bitScanForward(to_move_rooks);
    U64 moves = Tables::getRookAttacks(from_square, both_occupancies) & ~to_move_occupancies;
    while (moves)
    {
      int to_square = Utils::bitScanForward(moves);
      move_list.push_back(Move(from_square, to_square, ROOK, 0, Utils::getBit(opponent_occupancies, to_square), false, false, false));
      Utils::popLastBit(moves);
    }
    Utils::popLastBit(to_move_rooks);
  }
}

void generateQueenMoves(MoveList &move_list, U64 to_move_queens, U64 to_move_occupancies, U64 opponent_occupancies, U64 both_occupancies)
{
  while (to_move_queens)
  {
    int from_square = Utils::bitScanForward(to_move_queens);
    U64 moves = Tables::getQueenAttacks(from_square, both_occupancies) & ~to_move_occupancies;
    while (moves)
    {
      int to_square = Utils::bitScanForward(moves);
      move_list.push_back(Move(from_square, to_square, QUEEN, 0, Utils::getBit(opponent_occupancies, to_square), false, false, false));
      Utils::popLastBit(moves);
    }
    Utils::popLastBit(to_move_queens);
  }
}

void generateKingMoves(MoveList &move_list, U64 to_move_king, U64 to_move_occupancies, U64 opponent_occupancies)
{
  int from_square = Utils::bitScanForward(to_move_king);
  U64 moves = Tables::ATTACKS_KING[from_square] & ~to_move_occupancies;
  while (moves)
  {
    int to_square = Utils::bitScanForward(moves);
    move_list.push_back(Move(from_square, to_square, KING, 0, Utils::getBit(opponent_occupancies, to_square), false, false, false));
    Utils::popLastBit(moves);
  }
}