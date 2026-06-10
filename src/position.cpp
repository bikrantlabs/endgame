#include "position.h"
#include "types.h"
#include <iostream>

void Position::clear() {
  for (int c = 0; c < COLOR_NB; ++c)
    for (int p = 0; p < PIECE_TYPE_NB; ++p)
      pieces[c][p] = 0ULL;
  occ[WHITE] = occ[BLACK] = all_occ = 0ULL;

  side_to_move = WHITE;
  castling_rights = 0b1111; // all rights
  ep_square = NO_SQUARE;
  halfmove_clock = 0;
  fullmove_number = 1;
}

// Place piece of type pt, of color c in square
void Position::place_piece(Color c, PieceType pt, int sq) {

  Bitboard bit = sq_bb(sq);

  pieces[c][pt] = pieces[c][pt] | bit;
  occ[c] = occ[c] | bit;
  all_occ |= bit;
}
void Position::remove_piece(Color c, PieceType pt, int sq) {

  Bitboard bit = sq_bb(sq);

  pieces[c][pt] &= ~bit;
  occ[c] &= ~bit;
  all_occ &= ~bit;
}

void Position::move_piece(Color c, PieceType pt, int from, int to) {

  Bitboard mask = sq_bb(from) | sq_bb(to);

  pieces[c][pt] ^= mask;
  occ[c] ^= mask;
  all_occ ^= mask;
}

PieceType Position::piece_on(Color c, int s) const {
  Bitboard bit = sq_bb(s);
  for (int pt = 0; pt < PIECE_TYPE_NB; ++pt)
    if (pieces[c][pt] & bit)
      return static_cast<PieceType>(pt);

  return PIECE_TYPE_NB;
}

Color Position::color_on(int sq) const {
  Bitboard bit = sq_bb(sq);
  if (occ[WHITE] & bit)
    return WHITE;
  if (occ[BLACK] & bit)
    return BLACK;
  return COLOR_NB;
}

// Standard starting position
void Position::set_startpos() {
  clear();

  // White pieces (rank 1 & 2)
  place_piece(WHITE, ROOK, A1);
  place_piece(WHITE, ROOK, H1);
  place_piece(WHITE, KNIGHT, B1);
  place_piece(WHITE, KNIGHT, G1);
  place_piece(WHITE, BISHOP, C1);
  place_piece(WHITE, BISHOP, F1);
  place_piece(WHITE, QUEEN, D1);
  place_piece(WHITE, KING, E1);
  for (int f = 0; f < 8; ++f)
    place_piece(WHITE, PAWN, sq_of(f, 1));

  // Black pieces (rank 7 & 8)
  place_piece(BLACK, ROOK, A8);
  place_piece(BLACK, ROOK, H8);
  place_piece(BLACK, KNIGHT, B8);
  place_piece(BLACK, KNIGHT, G8);
  place_piece(BLACK, BISHOP, C8);
  place_piece(BLACK, BISHOP, F8);
  place_piece(BLACK, QUEEN, D8);
  place_piece(BLACK, KING, E8);
  for (int f = 0; f < 8; ++f)
    place_piece(BLACK, PAWN, sq_of(f, 6));
}

void Position::make_move(Move m) {
  int from = move_from(m);
  int to = move_to(m);
  int flag = move_flag(m);

  Color us = side_to_move;
  Color them = static_cast<Color>(1 - us);
  PieceType pt = piece_on(us, from);

  assert(pt != PIECE_TYPE_NB && "make_move: no piece on choosen square");

  if (is_capture(m)) {
    if (flag == EP_CAPTURE) {
      int ep_sq = (us == WHITE) ? to - 8 : to + 8; // enpassant
      remove_piece(them, PAWN, ep_sq);
    } else {
      PieceType captured = piece_on(them, to);
      remove_piece(them, captured, to);
    }
  }

  move_piece(us, pt, from, to);
  // Track en-passant square.
  ep_square = NO_SQUARE;

  // if opponent has make double push, ep_square is diagonal movement.
  if (flag == DOUBLE_PUSH) {
    ep_square = (us == WHITE) ? from + 8 : from - 8;
  }

  halfmove_clock = (pt == PAWN) ? 0 : halfmove_clock + 1;
  if (us == BLACK)
    ++fullmove_number;

  // Now it's opponent's turn
  side_to_move = static_cast<Color>(1 - us);
}

// Simple unmake for quiet moves — caller provides the moved piece type
void Position::unmake_move(Move m, PieceType moved_pt) {
  int from = move_from(m);
  int to = move_to(m);
  Color us = static_cast<Color>(1 - side_to_move); // who just moved

  move_piece(us, moved_pt, to, from); // reverse the move

  if (us == BLACK)
    --fullmove_number;
  side_to_move = us;
  ep_square = NO_SQUARE; // simplified: we'll track this properly later
}

static char piece_char(Color c, PieceType pt) {
  // Uppercase = White, lowercase = Black
  const char base[] = "PNBRQK";
  char ch = base[pt];
  if (c == BLACK)
    ch = static_cast<char>(ch + 32); // tolower
  return ch;
}

void print_board(const Position &pos, Bitboard highlight) {
  std::cout << "\n    a   b   c   d   e   f   g   h\n";
  std::cout << "  +---+---+---+---+---+---+---+---+\n";

  for (int rank = 7; rank >= 0; --rank) {
    std::cout << (rank + 1) << " |";
    for (int file = 0; file < 8; ++file) {
      int sq = sq_of(file, rank);
      bool hi = test_bit(highlight, sq);

      char display = '.';
      Color c = pos.color_on(sq);
      if (c != COLOR_NB) {
        PieceType pt = pos.piece_on(c, sq);
        display = piece_char(c, pt);
      }

      if (hi) {
        // Highlighted squares: show '*' if empty, piece char if occupied
        if (display == '.')
          std::cout << " * |";
        else
          std::cout << '(' << display << ")|";
      } else {
        std::cout << ' ' << display << " |";
      }
    }
    std::cout << " " << (rank + 1) << "\n";
    std::cout << "  +---+---+---+---+---+---+---+---+\n";
  }
  std::cout << "    a   b   c   d   e   f   g   h\n\n";
}
