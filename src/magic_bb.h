#pragma once
#include "types.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Magic Bitboards
//
//  HOW IT WORKS (short version):
//
//  For a rook on e4, only a specific set of squares can block it — the squares
//  along its four rays, excluding the board edges (edges can't block further).
//  That set is called the "mask". It has ~10 bits set.
//
//  With 10 bits there are 2^10 = 1024 possible occupancy patterns. For each
//  pattern the rook's real attacks are fixed and precomputed.
//
//  To index into that table at runtime:
//      occ   = all_occ & mask[sq]         // keep only the relevant bits
//      index = (occ * magic[sq]) >> shift[sq]  // hash to 0..table_size-1
//      attacks = TABLE[sq][index]          // one lookup, done
//
//  The "magic number" is a constant chosen so the multiply+shift maps every
//  possible masked occupancy to a unique (or harmlessly colliding) table slot.
//  The numbers below are well-known, published values — you don't derive them.
// ─────────────────────────────────────────────────────────────────────────────

// ── Table sizes ──────────────────────────────────────────────────────────────
// Rook masks have 10-12 relevant bits  → up to 4096 entries per square
// Bishop masks have 5-9 relevant bits  → up to 512 entries per square
static constexpr int ROOK_TABLE_SIZE = 4096;
static constexpr int BISHOP_TABLE_SIZE = 512;

// ── The actual attack tables (filled by init_magic_bitboards()) ──────────────
extern Bitboard ROOK_ATTACKS[64][ROOK_TABLE_SIZE];
extern Bitboard BISHOP_ATTACKS[64][BISHOP_TABLE_SIZE];

// ── Per-square metadata
// ───────────────────────────────────────────────────────
extern Bitboard ROOK_MASKS[64];   // which squares can block a rook on sq
extern Bitboard BISHOP_MASKS[64]; // which squares can block a bishop on sq

// ── Well-known magic numbers (one per square, rook and bishop)
// ──────────────── Source:
// https://www.chessprogramming.org/Best_Magic_Bitboards These are fixed
// constants — treat them as a lookup table.
static constexpr Bitboard ROOK_MAGICS[64] = {
    0x6080008020104000ULL, 0x40200010004002ULL,   0x10020004011000aULL,
    0x500050020100008ULL,  0x1500100500420800ULL, 0x300110002080400ULL,
    0x480220000800100ULL,  0x2000240820d0824ULL,  0x800080204004ULL,
    0x200400040201002ULL,  0x8808010002000ULL,    0x200801000800804ULL,
    0x10808008008400ULL,   0x102000200041108ULL,  0x801000401000200ULL,
    0x2200008104284aULL,   0x8580208008904000ULL, 0x200464000201008ULL,
    0x828010002000ULL,     0x9004420008120022ULL, 0x300808004000800ULL,
    0x902008002800400ULL,  0x850040008108201ULL,  0x220014488104ULL,
    0x40800040c0012010ULL, 0x2a40008480200042ULL, 0x6001001100402000ULL,
    0x10008080080012ULL,   0x4000080080800400ULL, 0x102020080040080ULL,
    0x2821400900138ULL,    0x300f204200008104ULL, 0x180004000402008ULL,
    0x8800200080804004ULL, 0x80801000802004ULL,   0x4080100a02004020ULL,
    0x80080800402ULL,      0x10800400800200ULL,   0x20804001001ULL,
    0x880010a902000044ULL, 0x1a00400080208000ULL, 0x400830040030020ULL,
    0x2058100020008080ULL, 0x2300090010010020ULL, 0x14008040080800ULL,
    0x22000408020010ULL,   0x121000200010004ULL,  0xa400804400820001ULL,
    0xa24080150100ULL,     0x4200804000200080ULL, 0x1002001020804200ULL,
    0xc204100302a00900ULL, 0x2108048001c0180ULL,  0x86800400020080ULL,
    0x5038011002080400ULL, 0x51058841200ULL,      0x201028000106941ULL,
    0x1004020801202ULL,    0x4001100c1200039ULL,  0x204040900201001ULL,
    0x800600200834508aULL, 0x1051000400080203ULL, 0x2000804016082ULL,
    0x80840080410022ULL};

static constexpr Bitboard BISHOP_MAGICS[64] = {
    0x10440088014309ULL,   0x190300100409114ULL,  0x310308a10400028ULL,
    0x1022a0200100202ULL,  0x22021030000200ULL,   0x10020804c4021008ULL,
    0xe002021017480400ULL, 0x1001008044200402ULL, 0x3000302182028208ULL,
    0x800100400b08200ULL,  0x2008211a04004800ULL, 0x8208b240600ULL,
    0x43000404203a0020ULL, 0x440011022500020ULL,  0x20010801042000ULL,
    0x200020084010804ULL,  0x140000504880200ULL,  0x408408a0a9c0c00ULL,
    0x2810b002812049ULL,   0x9204c04028012ULL,    0x84000a10220240ULL,
    0x1c100202008400ULL,   0x2084840108882100ULL, 0x91008440a0109ULL,
    0x202404420440430ULL,  0x8004200004010400ULL, 0x21280004080128ULL,
    0x4040020110010ULL,    0x2229001001004029ULL, 0x5010002100c80ULL,
    0xa081420804008410ULL, 0x832102a43808800ULL,  0x282882000c022a0ULL,
    0x8104f00420100404ULL, 0x2408104c01080800ULL, 0x9020080280080ULL,
    0x2880e0400001010ULL,  0x4a0440100002088ULL,  0xa010421040020101ULL,
    0x28800a080310040ULL,  0x5810110d0044200ULL,  0x49048814002140ULL,
    0x4416010402000100ULL, 0x40042218022400ULL,   0x1808080100400401ULL,
    0x21200800424280ULL,   0xc111011800821105ULL, 0x14880600281040ULL,
    0x1004c0288404100ULL,  0x5042082280802ULL,    0x4020042084400ULL,
    0x614420084110040ULL,  0x6804401282020411ULL, 0xa00803280a0100ULL,
    0x40888200821008ULL,   0x1022021204210800ULL, 0x1048044200400ULL,
    0x944904100300ULL,     0xc128009821084808ULL, 0x2000201002420620ULL,
    0x204048205150400ULL,  0x100002008010840ULL,  0x6060802140c01ULL,
    0x1202200101210100ULL};

// Shift = 64 - popcount(mask).  Precomputed here to avoid runtime popcount.
static constexpr int ROOK_SHIFTS[64] = {
    52, 53, 53, 53, 53, 53, 53, 52, 53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53, 53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53, 53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53, 52, 53, 53, 53, 53, 53, 53, 52,
};

static constexpr int BISHOP_SHIFTS[64] = {
    58, 59, 59, 59, 59, 59, 59, 58, 59, 59, 59, 59, 59, 59, 59, 59,
    59, 59, 57, 57, 57, 57, 59, 59, 59, 59, 57, 55, 55, 57, 59, 59,
    59, 59, 57, 55, 55, 57, 59, 59, 59, 59, 57, 57, 57, 57, 59, 59,
    59, 59, 59, 59, 59, 59, 59, 59, 58, 59, 59, 59, 59, 59, 59, 58,
};

// ── One-time init (call from init_movegen)
// ────────────────────────────────────
void init_magic_bitboards();

// ── Runtime lookup — call these in your generator functions ──────────────────

// Returns all squares a rook on `sq` attacks given board occupancy `occ`.
inline Bitboard rook_attacks(int sq, Bitboard occ) {
  occ &= ROOK_MASKS[sq];
  occ = (occ * ROOK_MAGICS[sq]) >> ROOK_SHIFTS[sq];
  return ROOK_ATTACKS[sq][occ];
}

// Returns all squares a bishop on `sq` attacks given board occupancy `occ`.
inline Bitboard bishop_attacks(int sq, Bitboard occ) {
  occ &= BISHOP_MASKS[sq];
  occ = (occ * BISHOP_MAGICS[sq]) >> BISHOP_SHIFTS[sq];
  return BISHOP_ATTACKS[sq][occ];
}

// Queen is just both combined — no separate table needed.
inline Bitboard queen_attacks(int sq, Bitboard occ) {
  return rook_attacks(sq, occ) | bishop_attacks(sq, occ);
}

/**
Methods to find the magic numbers.
// Add to magic_bb.h
Bitboard compute_rook_mask(int sq);
Bitboard compute_bishop_mask(int sq);
Bitboard compute_rook_attacks_slow(int sq, Bitboard blockers);
Bitboard compute_bishop_attacks_slow(int sq, Bitboard blockers);

void find_all_magics();
*/