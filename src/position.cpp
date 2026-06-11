#include "position.h"
#include "magic_bb.h"
#include "movegen.h"
#include "types.h"
#include "utils.h"
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

void Position::make_move(Move m, StateInfo &st) {
  //  Save state
  st.move = m;
  st.moved_pt = PIECE_TYPE_NB;
  st.captured = PIECE_TYPE_NB;
  st.captured_sq = NO_SQUARE;
  st.ep_square = ep_square;
  st.castling_rights = castling_rights;
  st.halfmove_clock = halfmove_clock;

  int from = move_from(m);
  int to = move_to(m);
  int flag = move_flag(m);

  Color us = side_to_move;
  Color them = static_cast<Color>(1 - us);
  PieceType pt = piece_on(us, from);

  assert(pt != PIECE_TYPE_NB && "make_move: no piece on choosen square");
  st.moved_pt = pt;

  //  Handle capture
  if (is_capture(m)) {
    if (flag == EP_CAPTURE) {
      st.captured = PAWN;
      st.captured_sq = (us == WHITE) ? to - 8 : to + 8;
      remove_piece(them, PAWN, st.captured_sq);
    } else {
      st.captured = piece_on(them, to);
      st.captured_sq = to;
      remove_piece(them, st.captured, to);
    }
  }

  //  Handle castling rook movement
  if (flag == KING_CASTLE) {
    int rook_from = (us == WHITE) ? H1 : H8;
    int rook_to = (us == WHITE) ? F1 : F8;
    move_piece(us, ROOK, rook_from, rook_to);
  } else if (flag == QUEEN_CASTLE) {
    int rook_from = (us == WHITE) ? A1 : A8;
    int rook_to = (us == WHITE) ? D1 : D8;
    move_piece(us, ROOK, rook_from, rook_to);
  }

  //  Move the piece
  move_piece(us, pt, from, to);

  //  Handle promotion (replace pawn with promoted piece)
  if (is_promotion(m)) {
    PieceType promo_pt = static_cast<PieceType>((flag & 3) + KNIGHT);
    remove_piece(us, PAWN, to);
    place_piece(us, promo_pt, to);
  }

  //  Update castling rights
  if (pt == KING) {
    castling_rights &=
        (us == WHITE) ? ~(WK_CASTLE | WQ_CASTLE) : ~(BK_CASTLE | BQ_CASTLE);
  }
  if (from == A1)
    castling_rights &= ~WQ_CASTLE;
  if (from == H1)
    castling_rights &= ~WK_CASTLE;
  if (from == A8)
    castling_rights &= ~BQ_CASTLE;
  if (from == H8)
    castling_rights &= ~BK_CASTLE;

  if (is_capture(m) && flag != EP_CAPTURE) {
    if (to == A1)
      castling_rights &= ~WQ_CASTLE;
    if (to == H1)
      castling_rights &= ~WK_CASTLE;
    if (to == A8)
      castling_rights &= ~BQ_CASTLE;
    if (to == H8)
      castling_rights &= ~BK_CASTLE;
  }

  //  En-passant
  ep_square = NO_SQUARE;
  if (flag == DOUBLE_PUSH)
    ep_square = (us == WHITE) ? from + 8 : from - 8;

  //  Clocks
  halfmove_clock = (pt == PAWN) ? 0 : halfmove_clock + 1;
  if (us == BLACK)
    ++fullmove_number;

  side_to_move = them;
}

bool Position::is_square_attacked(int sq, Color attacker_color) const {
  Bitboard pawns = pieces[attacker_color][PAWN];
  Bitboard bb = sq_bb(sq);
  Bitboard pawn_attacks = 0;
  // Shift that single square diagonally south or north and check if there are
  // PAWNS.
  if (attacker_color == WHITE) {
    pawn_attacks = (shift_se(bb) | shift_sw(bb)) & pawns;

  } else if (attacker_color == BLACK) {
    pawn_attacks = (shift_ne(bb) | shift_nw(bb)) & pawns;
  }

  // Check if the square is in attack of Knight AND those squares have
  // right-color knight.
  Bitboard knight_attacks = KNIGHT_ATTACKS[sq] & pieces[attacker_color][KNIGHT];

  // If the square is in range of bishop or queen attack AND those square has
  // either enemy bishop or queen
  Bitboard bishop_or_queen_attacks =
      bishop_attacks(sq, all_occ) &
      (pieces[attacker_color][BISHOP] | pieces[attacker_color][QUEEN]);

  // Similar to bishop_attacks
  Bitboard rook_or_queen_attacks =
      rook_attacks(sq, all_occ) &
      (pieces[attacker_color][ROOK] | pieces[attacker_color][QUEEN]);

  Bitboard king_attacks = KING_ATTACKS[sq] & pieces[attacker_color][KING];

  return pawn_attacks | knight_attacks | bishop_or_queen_attacks |
         rook_or_queen_attacks | king_attacks;
}

// Checks if the king is in check
bool Position::is_in_check() const {
  // Get king's square:
  int sq = Util::king_square(*this, side_to_move);

  Color enemy = static_cast<Color>(1 - side_to_move);

  return is_square_attacked(sq, enemy);
}
void Position::unmake_move(const StateInfo &st) {
  Move m = st.move;
  int from = move_from(m);
  int to = move_to(m);
  int flag = move_flag(m);

  Color us = static_cast<Color>(1 - side_to_move); // who just moved

  //  Reverse promotion (remove promoted piece, restore pawn) ──
  if (is_promotion(m)) {
    PieceType promo_pt = static_cast<PieceType>((flag & 3) + KNIGHT);
    remove_piece(us, promo_pt, to);
    place_piece(us, PAWN, to);
  }

  //  Reverse piece movement
  move_piece(us, st.moved_pt, to, from);

  //  Reverse castling rook
  if (flag == KING_CASTLE) {
    int rook_from = (us == WHITE) ? H1 : H8;
    int rook_to = (us == WHITE) ? F1 : F8;
    move_piece(us, ROOK, rook_to, rook_from);
  } else if (flag == QUEEN_CASTLE) {
    int rook_from = (us == WHITE) ? A1 : A8;
    int rook_to = (us == WHITE) ? D1 : D8;
    move_piece(us, ROOK, rook_to, rook_from);
  }

  //  Restore captured piece
  if (st.captured != PIECE_TYPE_NB)
    place_piece(static_cast<Color>(1 - us), st.captured, st.captured_sq);

  //  Restore state from snapshot
  ep_square = st.ep_square;
  castling_rights = st.castling_rights;
  halfmove_clock = st.halfmove_clock;
  side_to_move = us;

  if (us == BLACK)
    --fullmove_number;
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
