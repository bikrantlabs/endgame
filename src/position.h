#pragma once
#include "types.h"
// ─────────────────────────────────────────────
//  StateInfo — snapshot for unmake_move
// ─────────────────────────────────────────────
struct StateInfo {
  Move move;
  PieceType moved_pt; // PAWN for promotions (original piece)
  PieceType captured; // PIECE_TYPE_NB if no capture
  int captured_sq;    // where captured piece was (differs from `to` for EP)
  int ep_square;
  int castling_rights;
  int halfmove_clock;
};

// ─────────────────────────────────────────────
//  Position
//  Stores the complete board state using bitboards.
//  pieces[color][pieceType] — one bitboard per (color, piece) pair.
// ─────────────────────────────────────────────

class Position {
public:
  // 12 piece bitboards: pieces[WHITE][PAWN], pieces[BLACK][KING], etc.
  Bitboard pieces[COLOR_NB][PIECE_TYPE_NB];

  // Derived occupancy — always kept in sync with pieces[][]
  Bitboard occ[COLOR_NB]; // occ[WHITE] = all white pieces
  Bitboard all_occ;       // occ[WHITE] | occ[BLACK]

  // Game state
  Color side_to_move;
  int castling_rights; // 4 bits: bit0=WK, bit1=WQ, bit2=BK, bit3=BQ
  int ep_square;       // en passant target square, or NO_SQUARE
  int halfmove_clock;  // for 50-move rule
  int fullmove_number;

  // Clear everything
  void clear();

  // Load the standard starting position
  void set_startpos();

  // Place a piece; updates all occupancy boards and the piece board.
  void place_piece(Color c, PieceType pt, int sq);

  // Remove a piece from sq (caller must know what's there).
  void remove_piece(Color c, PieceType pt, int sq);

  // Move a piece from one square to another (quiet move only).
  void move_piece(Color c, PieceType pt, int from, int to);

  // Which piece type is on sq for color c? Returns PIECE_TYPE_NB if none.
  PieceType piece_on(Color c, int sq) const;

  // Which color's piece is on sq? Returns COLOR_NB if empty.
  Color color_on(int sq) const;

  // Is sq occupied by any piece?
  bool is_occupied(int sq) const { return test_bit(all_occ, sq); }

  // Is the square attacked by any piece of given color?
  bool is_square_attacked(int sq, Color c) const;

  // Is our king checked?
  bool is_in_check() const;

  // Make / Unmake

  void make_move(Move m, StateInfo &st);
  void unmake_move(const StateInfo &st);
};

// Print the board. If highlight != 0, marks those squares with '*'.
void print_board(const Position &pos, Bitboard highlight = 0ULL);