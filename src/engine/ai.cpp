#include "ai.hpp"

#include "utils.hpp"
#include "bitboard.hpp"
#include "move.hpp"
#include "board.hpp"
#include "movelist.hpp"
#include "movegen.hpp"
#include "eval.hpp"

#include <climits>

#define MIN_EVAL (INT_MIN + 1)

void AI::setDepth(int depth)
{
    _depth = depth;
}

int AI::getDepth() const
{
    return _depth;
}

AI::SearchResult AI::findBestMove()
{
    _nodes = 0;

    int alpha = MIN_EVAL;
    Move best_move = Move();
    MoveList move_list = _board.getSideToMove() == WHITE ? movegen::generatePseudoLegalMoves<WHITE>(_board) : movegen::generatePseudoLegalMoves<BLACK>(_board);
    for (const Move &move : move_list)
    {
        Board backup = _board;
        backup.makeMove(move);
        int king_sq = bitboard::bitScanForward(backup.getPieces(backup.getOpponent(), KING));
        int attacker_side = backup.getSideToMove();
        if (!backup.isSquareAttacked(king_sq, attacker_side))
        {
            int score = -search(MIN_EVAL, -alpha, _depth, backup);
            if (score > alpha)
            {
                alpha = score;
                best_move = move;
            }
        }
    }

    return SearchResult{alpha, best_move.getEncoded(), _nodes};
}

int AI::search(int alpha, int beta, int depth, const Board &board)
{
    _nodes++;

    if (depth == 0)
    {
        return eval::eval(board);
    }

    bool has_legal_moves = false;
    MoveList move_list = board.getSideToMove() == WHITE ? movegen::generatePseudoLegalMoves<WHITE>(board) : movegen::generatePseudoLegalMoves<BLACK>(board);
    for (const Move &move : move_list)
    {
        Board backup = board;
        backup.makeMove(move);
        int king_sq = bitboard::bitScanForward(backup.getPieces(backup.getOpponent(), KING));
        if (!backup.isSquareAttacked(king_sq, backup.getSideToMove()))
        {
            has_legal_moves = true;
            int score = -search(-beta, -alpha, depth - 1, backup);
            if (score >= beta)
            {
                return beta;
            }
            if (score > alpha)
            {
                alpha = score;
            }
        }
    }

    if (!has_legal_moves)
    {
        int king_sq = bitboard::bitScanForward(board.getPieces(board.getSideToMove(), KING));
        if (board.isSquareAttacked(king_sq, board.getOpponent()))
        {
            return MIN_EVAL + _depth - depth;
        }

        return 0;
    }

    return alpha;
}
