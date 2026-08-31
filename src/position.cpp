#include "position.h"
#include "magic_bb.h"
#include "movegen.h"
#include "types.h"
#include "utils.h"
#include "zobrist.h"
#include <algorithm>
#include <iostream>

static Move dbg_path[256];
static int dbg_depth = 0;
void dbg_path_push(Move m) {
  if (dbg_depth < 256)
    dbg_path[dbg_depth++] = m;
}
void dbg_path_pop() {
  if (dbg_depth > 0)
    dbg_depth--;
}
void dbg_path_reset() { dbg_depth = 0; }
void dbg_path_dump() {
  std::cerr << "dbg_depth=" << dbg_depth << " tail=[";
  int start = 0;
  if (dbg_depth > 14)
    start = dbg_depth - 14;
  for (int i = start; i < dbg_depth; i++)
    std::cerr << Util::move_to_string(dbg_path[i]) << " ";
  std::cerr << "]\n";
}

void Position::compute_hash() {
  hash = 0;

  // Every piece on every square
  for (int c = 0; c < COLOR_NB; ++c)
    for (int pt = 0; pt < PIECE_TYPE_NB; ++pt) {
      Bitboard bb = pieces[c][pt];
      while (bb) {
        int sq = unset_lsb(bb);
        hash ^= ZOBRIST.pieces[piece_of(static_cast<Color>(c),
                                        static_cast<PieceType>(pt))][sq];
      }
    }

  // Side to move (XOR in when it's black's turn)
  if (side_to_move == BLACK)
    hash ^= ZOBRIST.side;

  // Castling rights
  hash ^= ZOBRIST.castling[castling_rights];

  // En-passant file (only when an EP capture is actually possible)
  if (ep_square != NO_SQUARE)
    hash ^= ZOBRIST.ep[sq_file(ep_square)];
}

GameResult Position::game_result() {
  if (halfmove_clock >= 100)
    return DRAW_50_MOVES;

  MoveList ml;
  gen_legal_moves(*this, ml);
  if (ml.empty()) {
    if (is_in_check())
      return CHECKMATE;
    return STALEMATE;
  }

  int count = (int)std::count(game_history.begin(), game_history.end(), hash);
  if (count >= 3)
    return DRAW_REPETITION;

  return ONGOING;
}

void Position::clear() {
  for (int c = 0; c < COLOR_NB; ++c)
    for (int p = 0; p < PIECE_TYPE_NB; ++p)
      pieces[c][p] = 0ULL;
  occ[WHITE] = occ[BLACK] = all_occ = 0ULL;

  side_to_move = WHITE;
  castling_rights = 0b1111;
  ep_square = NO_SQUARE;
  halfmove_clock = 0;
  fullmove_number = 1;
  hash = 0;
  game_history.clear();
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

void Position::repair_consistency() {
  // Remove double-claimed squares: keep the lower piece-type index
  for (int c = 0; c < COLOR_NB; ++c) {
    for (int i = 0; i < PIECE_TYPE_NB; ++i) {
      for (int j = i + 1; j < PIECE_TYPE_NB; ++j) {
        Bitboard conflict = pieces[c][i] & pieces[c][j];
        if (conflict)
          pieces[c][j] &= ~conflict;
      }
    }
  }
  // Rebuild occ from cleaned pieces
  for (int c = 0; c < COLOR_NB; ++c) {
    Bitboard computed = 0;
    for (int pt = 0; pt < PIECE_TYPE_NB; ++pt)
      computed |= pieces[c][pt];
    occ[c] = computed;
  }
  all_occ = occ[WHITE] | occ[BLACK];
}

PieceType Position::piece_type_on(Color c, int s) const {
  Bitboard bit = sq_bb(s);
  for (int pt = 0; pt < PIECE_TYPE_NB; ++pt)
    if (pieces[c][pt] & bit)
      return static_cast<PieceType>(pt);

  return PIECE_TYPE_NB;
}

Piece Position::piece_on(int square) {
  PieceType pt = this->piece_type_on(WHITE, square);

  if (pt != PIECE_TYPE_NB) {
    return piece_of(WHITE, pt);
  }
  pt = this->piece_type_on(BLACK, square);
  if (pt != PIECE_TYPE_NB)
    return piece_of(BLACK, pt);

  return NO_PIECE;
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

  compute_hash();
  game_history.clear();
  game_history.push_back(hash);
}

void Position::make_move(Move m, StateInfo &st) {
  dbg_path_push(m);
  //  Save state
  st.move = m;
  st.moved_pt = PIECE_TYPE_NB;
  st.captured = PIECE_TYPE_NB;
  st.captured_sq = NO_SQUARE;
  st.ep_square = ep_square;
  st.castling_rights = castling_rights;
  st.halfmove_clock = halfmove_clock;
  st.hash = hash;

  int from = move_from(m);
  int to = move_to(m);
  int flag = move_flag(m);

  Color us = side_to_move;
  Color them = static_cast<Color>(1 - us);

  PieceType pt = piece_type_on(us, from);

  assert(pt != PIECE_TYPE_NB && "make_move: no piece on chosen square");
  assert(us != COLOR_NB && "make_move: no valid color");
  st.moved_pt = pt;

  Piece moving_piece = piece_of(us, pt);

  //   Castling rights and EP file must be removed before they are updated.
  hash ^= ZOBRIST.castling[castling_rights];
  if (ep_square != NO_SQUARE)
    hash ^= ZOBRIST.ep[sq_file(ep_square)];
  // XOR OUT the moving piece from its source square
  hash ^= ZOBRIST.pieces[moving_piece][from];

  //  Handle capture
  if (is_capture(m)) {
    if (flag == EP_CAPTURE) {
      st.captured = PAWN;
      st.captured_sq = (us == WHITE) ? to - 8 : to + 8;
      // XOR out the captured pawn from ITS square
      hash ^= ZOBRIST.pieces[piece_of(them, PAWN)][st.captured_sq];
      remove_piece(them, PAWN, st.captured_sq);
    } else {
      st.captured = piece_type_on(them, to);
      if (st.captured == PIECE_TYPE_NB) {
        std::cerr << "[BADCAP] flag=" << flag << " from=" << from << " to=" << to
                  << " side=" << us << "\n";
        print_board(*this, 0);
        std::exit(1);
      }
      st.captured_sq = to;
      // XOR out the captured piece from the destination square
      hash ^= ZOBRIST.pieces[piece_of(them, st.captured)][to];
      remove_piece(them, st.captured, to);
    }
  }

  //  Handle castling rook movement
  if (flag == KING_CASTLE) {
    int rook_from = (us == WHITE) ? H1 : H8;
    int rook_to = (us == WHITE) ? F1 : F8;
    hash ^= ZOBRIST.pieces[piece_of(us, ROOK)][rook_from];
    hash ^= ZOBRIST.pieces[piece_of(us, ROOK)][rook_to];
    remove_piece(us, ROOK, rook_from);
    place_piece(us, ROOK, rook_to);
  } else if (flag == QUEEN_CASTLE) {
    int rook_from = (us == WHITE) ? A1 : A8;
    int rook_to = (us == WHITE) ? D1 : D8;
    hash ^= ZOBRIST.pieces[piece_of(us, ROOK)][rook_from];
    hash ^= ZOBRIST.pieces[piece_of(us, ROOK)][rook_to];
    remove_piece(us, ROOK, rook_from);
    place_piece(us, ROOK, rook_to);
  }

  //  Move the piece
  remove_piece(us, pt, from);
  place_piece(us, pt, to);

  //  Handle promotion (replace pawn with promoted piece)
  if (is_promotion(m)) {
    PieceType promo_pt = get_promotion_piece(m);
    // The pawn moved to 'to' during move_piece() call; now swap it for the
    // promoted piece
    hash ^= ZOBRIST.pieces[piece_of(us, promo_pt)][to];
    remove_piece(us, PAWN, to);
    place_piece(us, promo_pt, to);
  } else {
    hash ^= ZOBRIST.pieces[moving_piece][to];
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

  // XOR IN the new volatile state
  hash ^= ZOBRIST.castling[castling_rights];
  if (ep_square != NO_SQUARE)
    hash ^= ZOBRIST.ep[sq_file(ep_square)];

  // Flip side to move
  hash ^= ZOBRIST.side;

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
  dbg_path_pop();
  Move m = st.move;
  int from = move_from(m);
  int to = move_to(m);
  int flag = move_flag(m);

  Color us = static_cast<Color>(1 - side_to_move); // who just moved

  //  Reverse promotion (remove promoted piece, restore pawn) ──
  if (is_promotion(m)) {
    PieceType promo_pt = get_promotion_piece(m);
    remove_piece(us, promo_pt, to);
    place_piece(us, PAWN, to);
  }

  //  Reverse piece movement (use remove+place, NOT move_piece XOR,
  //  to be robust against child-search corruption of unrelated bits)
  remove_piece(us, st.moved_pt, to);
  place_piece(us, st.moved_pt, from);

  //  Reverse castling rook
  if (flag == KING_CASTLE) {
    int rook_from = (us == WHITE) ? H1 : H8;
    int rook_to = (us == WHITE) ? F1 : F8;
    remove_piece(us, ROOK, rook_to);
    place_piece(us, ROOK, rook_from);
  } else if (flag == QUEEN_CASTLE) {
    int rook_from = (us == WHITE) ? A1 : A8;
    int rook_to = (us == WHITE) ? D1 : D8;
    remove_piece(us, ROOK, rook_to);
    place_piece(us, ROOK, rook_from);
  }

  //  Restore captured piece
  if (st.captured != PIECE_TYPE_NB)
    place_piece(static_cast<Color>(1 - us), st.captured, st.captured_sq);

  //  Restore state from snapshot
  ep_square = st.ep_square;
  castling_rights = st.castling_rights;
  halfmove_clock = st.halfmove_clock;
  side_to_move = us;
  hash = st.hash;

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
        PieceType pt = pos.piece_type_on(c, sq);
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
