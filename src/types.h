#pragma once
#include <cassert>
#include <cstdint>
#include <string>

using Bitboard = uint64_t;
using ZobrishKey = uint64_t;
using Move = uint16_t; // bits 0-5: from | 6-11: to | 12-15: flags

constexpr int NUM_SQUARES = 64;
constexpr int NUM_FILES = 8;
constexpr int NUM_RANKS = 8;

// Rank / File masks
/**
      a    b    c    d    e    f    g    h
   ┌────┬────┬────┬────┬────┬────┬────┬────┐
8  │ 56 │ 57 │ 58 │ 59 │ 60 │ 61 │ 62 │ 63 │
   ├────┼────┼────┼────┼────┼────┼────┼────┤
7  │ 48 │ 49 │ 50 │ 51 │ 52 │ 53 │ 54 │ 55 │
   ├────┼────┼────┼────┼────┼────┼────┼────┤
6  │ 40 │ 41 │ 42 │ 43 │ 44 │ 45 │ 46 │ 47 │
   ├────┼────┼────┼────┼────┼────┼────┼────┤
5  │ 32 │ 33 │ 34 │ 35 │ 36 │ 37 │ 38 │ 39 │
   ├────┼────┼────┼────┼────┼────┼────┼────┤
4  │ 24 │ 25 │ 26 │ 27 │ 28 │ 29 │ 30 │ 31 │
   ├────┼────┼────┼────┼────┼────┼────┼────┤
3  │ 16 │ 17 │ 18 │ 19 │ 20 │ 21 │ 22 │ 23 │
   ├────┼────┼────┼────┼────┼────┼────┼────┤
2  │  8 │  9 │ 10 │ 11 │ 12 │ 13 │ 14 │ 15 │
   ├────┼────┼────┼────┼────┼────┼────┼────┤
1  │  0 │  1 │  2 │  3 │  4 │  5 │  6 │  7 │
   └────┴────┴────┴────┴────┴────┴────┴────┘
Rank 1 = Bottom Rank all 1s: RANK_1 = 0x00000000000000FF
File A = 0,8,16,24,32,40,48,56 bits 1s(every 8 bits 1) = 0x0101010101010101
*/
constexpr Bitboard RANK_1 = 0x00000000000000FFULL;
constexpr Bitboard RANK_2 = 0x000000000000FF00ULL;
constexpr Bitboard RANK_3 = 0x0000000000FF0000ULL;
constexpr Bitboard RANK_4 = 0x00000000FF000000ULL;
constexpr Bitboard RANK_5 = 0x000000FF00000000ULL;
constexpr Bitboard RANK_6 = 0x0000FF0000000000ULL;
constexpr Bitboard RANK_7 = 0x00FF000000000000ULL;
constexpr Bitboard RANK_8 = 0xFF00000000000000ULL;

constexpr Bitboard FILE_A = 0x0101010101010101ULL;
constexpr Bitboard FILE_B = 0x0202020202020202ULL;
constexpr Bitboard FILE_C = 0x0404040404040404ULL;
constexpr Bitboard FILE_D = 0x0808080808080808ULL;
constexpr Bitboard FILE_E = 0x1010101010101010ULL;
constexpr Bitboard FILE_F = 0x2020202020202020ULL;
constexpr Bitboard FILE_G = 0x4040404040404040ULL;
constexpr Bitboard FILE_H = 0x8080808080808080ULL;

constexpr Bitboard NOT_FILE_A = ~FILE_A;
constexpr Bitboard NOT_FILE_H = ~FILE_H;

enum Color { WHITE = 0, BLACK = 1, COLOR_NB = 2 };

enum PieceType {
  PAWN = 0,
  KNIGHT = 1,
  BISHOP = 2,
  ROOK = 3,
  QUEEN = 4,
  KING = 5,
  PIECE_TYPE_NB = 6
};

// Piece = color * 6 + type  (used for display / PST later)
enum Piece {
  W_PAWN,
  W_KNIGHT,
  W_BISHOP,
  W_ROOK,
  W_QUEEN,
  W_KING,
  B_PAWN,
  B_KNIGHT,
  B_BISHOP,
  B_ROOK,
  B_QUEEN,
  B_KING,
  NO_PIECE,
  PIECE_NB
};
// Squares — a1=0 ... h8=63
enum Square : int {
  A1,
  B1,
  C1,
  D1,
  E1,
  F1,
  G1,
  H1,
  A2,
  B2,
  C2,
  D2,
  E2,
  F2,
  G2,
  H2,
  A3,
  B3,
  C3,
  D3,
  E3,
  F3,
  G3,
  H3,
  A4,
  B4,
  C4,
  D4,
  E4,
  F4,
  G4,
  H4,
  A5,
  B5,
  C5,
  D5,
  E5,
  F5,
  G5,
  H5,
  A6,
  B6,
  C6,
  D6,
  E6,
  F6,
  G6,
  H6,
  A7,
  B7,
  C7,
  D7,
  E7,
  F7,
  G7,
  H7,
  A8,
  B8,
  C8,
  D8,
  E8,
  F8,
  G8,
  H8,
  NO_SQUARE = -1
};

// Move flags
enum MoveFlag : uint16_t {
  QUIET = 0,
  DOUBLE_PUSH = 1,
  KING_CASTLE = 2,
  QUEEN_CASTLE = 3,
  CAPTURE = 4,
  EP_CAPTURE = 5,
  PROMO_N = 8,      // Promotion to Knight
  PROMO_B = 9,      // Promotion to Bishop
  PROMO_R = 10,     // Promotion to Rook
  PROMO_Q = 11,     // Promotion to Queen
  PROMO_CAP_N = 12, // Captures and Promotes to Knight
  PROMO_CAP_B = 13,
  PROMO_CAP_R = 14,
  PROMO_CAP_Q = 15
};

enum CastlingSide { KING_SIDE, QUEEN_SIDE, CASTLING_SIDE_NB = 2 };

enum CastlingRight : int {
  WK_CASTLE = 1 << 0, // 0b0001
  WQ_CASTLE = 1 << 1, // 0b0010
  BK_CASTLE = 1 << 2, // 0b0100
  BQ_CASTLE = 1 << 3, // 0b1000
};

/// Make move from starting square to destination square with flag.
/// @returns -  16-bit encoded move
inline Move make_move(int from, int to, MoveFlag flag = QUIET) {
  return static_cast<Move>((flag << 12) | (to << 6) | from);
}

/// Extract the "from" and "to" bits from the Move and return as Square
/// Just perform AND Operation with 6 "1s" and rest 0s.
/// The result will be extracted 6-bits
/// 111111 = 3F in hex
inline int move_from(Move m) { return (m & 0x3F); }

inline int move_to(Move m) { return ((m >> 6) & 0x3F); }

// No need to do "AND" here, since all other bits are already cleared.
inline int move_flag(Move m) { return ((m >> 12)); }

inline bool is_capture(Move m) { return (move_flag(m) & CAPTURE) != 0; }

inline bool is_promotion(Move m) { return move_flag(m) >= PROMO_N; }

/**
PROMO_N=8    (1000) → 8&3=0 +1 = Knight
PROMO_B=9    (1001) → 9&3=1 +1 = Bishop
PROMO_R=10   (1010) → 10&3=2+1 = Rook
PROMO_Q=11   (1011) → 11&3=3+1 = Queen
 */
inline PieceType get_promotion_piece(Move m) {
  int flag = move_flag(m);
  return static_cast<PieceType>((flag & 3) + KNIGHT);
}

// Square helpers
/// Get the file index of Square s
/// Eg: Square G4(30) -> File 6(which is g file)
inline int sq_file(int s) { return s & 7; }

/// Get the rank index of Square s
/// Eg: Square G4(30) -> Rank 3 (which is 4th rank)
inline int sq_rank(int s) { return s >> 3; }

/// Get the square number(0-63) for specific file and rank
inline int sq_of(int file, int rank) { return (rank * 8 + file); }

/// Parse "e2" -> Square index(0-63). Returns NO_SQUARE on bad input.
inline int parse_square(const std::string &s) {
  if (s.size() < 2)
    return NO_SQUARE; // Single letter input
  int file = s[0] - 'a';
  int rank = s[1] - '1';
  if (file < 0 || file > 7 || rank < 0 || rank > 7)
    return NO_SQUARE;
  return sq_of(file, rank);
}

/// Get name from square: 30 -> g4
inline std::string sq_name(int s) {
  if (s == NO_SQUARE)
    return "--";
  std::string name;

  name += static_cast<char>('a' + sq_file(s));
  name += static_cast<char>('1' + sq_rank(s));
  return name;
}

inline Piece piece_of(Color color, PieceType type) {
  // Safety checks
  assert(color == WHITE || color == BLACK);
  assert(type >= PAWN && type <= KING);

  int piece = color * 6 + type;
  return static_cast<Piece>(piece);
}

/// Convert square to bitboard mask
inline Bitboard sq_bb(int s) { return 1ULL << s; }

/// Set bit at specific square( meaning that there is a piece.)
inline void set_bit(Bitboard &b, int sq) { b = b | sq_bb(sq); }

/// Clear bit at specific square( meaning that there is no piece.)
inline void clear_bit(Bitboard &b, int sq) { b = b & ~sq_bb(sq); }

/// Check if the square is set.
inline bool test_bit(Bitboard b, int sq) { return (b >> sq) & 1; }

/// Get the index of Lowest set bit square
/// Example: b = 00101000; index = 3.
inline int lsb(Bitboard b) {
  assert(b != 0);
  return __builtin_ctzll(b);
}

/// Unset lowest set bit, return its index
inline int unset_lsb(Bitboard &b) {
  int sq = lsb(b);
  b &= b - 1;
  return sq;
}

/// Get total 1s in the board
inline int popcount(Bitboard b) { return __builtin_popcountll(b); }

// Shifting north(white direction) / south (black direction)
inline Bitboard shift_north(Bitboard b) { return b << 8; }
inline Bitboard shift_south(Bitboard b) { return b >> 8; }

/// Diagonals shifting
inline Bitboard shift_ne(Bitboard b) { return (b & NOT_FILE_H) << 9; }
inline Bitboard shift_nw(Bitboard b) { return (b & NOT_FILE_A) << 7; }
inline Bitboard shift_se(Bitboard b) { return (b & NOT_FILE_H) >> 7; }
inline Bitboard shift_sw(Bitboard b) { return (b & NOT_FILE_A) >> 9; }

struct MoveList {
  Move moves[256];
  int count = 0;

  void add(Move m) {
    assert(count < 256 && "MoveList overflow");
    moves[count++] = m;
  }
  void clear() { count = 0; }
  bool empty() const { return count == 0; }
  Move operator[](int i) const { return moves[i]; }
};