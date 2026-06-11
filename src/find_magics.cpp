// #include "magic_bb.h"
// #include "types.h"
// #include <cstdio>
// #include <cstdlib>
// #include <cstring>
// #include <ctime>

// // Simple xorshift64 random number generator
// static uint64_t seed = 1070372ull;
// static uint64_t rand64() {
//   seed ^= seed << 13;
//   seed ^= seed >> 7;
//   seed ^= seed << 17;
//   return seed;
// }

// // Sparse random — magic numbers work better with few set bits
// static uint64_t sparse_rand() { return rand64() & rand64() & rand64(); }

// // Enumerate all subsets of mask (carry-rippler)
// static int all_subsets(Bitboard mask, Bitboard *subsets) {
//   int count = 0;
//   Bitboard sub = 0;
//   do {
//     subsets[count++] = sub;
//     sub = (sub - mask) & mask;
//   } while (sub != 0);
//   return count;
// }

// static Bitboard find_magic(int sq, bool is_rook) {
//   Bitboard mask = is_rook ? compute_rook_mask(sq) : compute_bishop_mask(sq);
//   int bits = popcount(mask);
//   int shift = 64 - bits;
//   int size = 1 << bits;

//   Bitboard subsets[4096];
//   Bitboard attacks[4096];
//   Bitboard used[4096];

//   int n = all_subsets(mask, subsets);

//   // Precompute correct attacks for every subset
//   for (int i = 0; i < n; i++)
//     attacks[i] = is_rook ? compute_rook_attacks_slow(sq, subsets[i])
//                          : compute_bishop_attacks_slow(sq, subsets[i]);

//   // Try random candidates until one works
//   for (int attempt = 0; attempt < 100000000; attempt++) {
//     Bitboard magic = sparse_rand();

//     // Quick filter: magic must map the mask to the upper bits
//     if (popcount((mask * magic) >> 56) < 6)
//       continue;

//     memset(used, 0, sizeof(Bitboard) * size);
//     bool ok = true;

//     for (int i = 0; i < n; i++) {
//       int idx = (int)((subsets[i] * magic) >> shift);
//       if (used[idx] == 0)
//         used[idx] = attacks[i];
//       else if (used[idx] != attacks[i]) {
//         ok = false;
//         break;
//       }
//     }
//     if (ok)
//       return magic;
//   }

//   printf("FAILED to find magic for sq=%d\n", sq);
//   return 0ULL;
// }

// void find_all_magics() {
//   printf("ROOK MAGICS:\n");
//   for (int sq = 0; sq < 64; sq++) {
//     Bitboard m = find_magic(sq, true);
//     printf("0x%llxULL,\n", (unsigned long long)m);
//   }

//   printf("\nBISHOP MAGICS:\n");
//   for (int sq = 0; sq < 64; sq++) {
//     Bitboard m = find_magic(sq, false);
//     printf("0x%llxULL,\n", (unsigned long long)m);
//   }
// }
