/**
 * A fast chess library for C++
 */
#pragma once
#include "defs.hpp"
#include <array>
#include <immintrin.h> // TODO: Do not always assume __has_include(<immintrin.h>)

// TODO: Make actually good
namespace chess {
	namespace lookup {
		// LUT that maps each square to a knight attack bitboard
		alignas(64) inline constexpr std::array<Bitboard, 64> knightAttacks = ([]() constexpr {
			std::array<Bitboard, 64> output;

			for (int i = 0; i < 64; ++i) {
				const Bitboard spot = 1ull << i;

				output[i] =
					/* (+1, +2) */ ((spot & ~(RankMask::Rank7 | RankMask::Rank8 | FileMask::hFile)) << 17) |
					/* (-1, +2) */ ((spot & ~(RankMask::Rank7 | RankMask::Rank8 | FileMask::aFile)) << 15) |
					/* (-1, -2) */ ((spot & ~(RankMask::Rank1 | RankMask::Rank2 | FileMask::aFile)) >> 17) |
					/* (+1, -2) */ ((spot & ~(RankMask::Rank1 | RankMask::Rank2 | FileMask::hFile)) >> 15) |
					/* (+2, +1) */ ((spot & ~(RankMask::Rank8 | FileMask::gFile | FileMask::hFile)) << 10) |
					/* (-2, +1) */ ((spot & ~(RankMask::Rank8 | FileMask::aFile | FileMask::bFile)) << 6) |
					/* (-2, -1) */ ((spot & ~(RankMask::Rank1 | FileMask::aFile | FileMask::bFile)) >> 10) |
					/* (+2, -1) */ ((spot & ~(RankMask::Rank1 | FileMask::gFile | FileMask::hFile)) >> 6);
			}

			return output;
		})();

		// LUT that maps each square to a king attack bitboard
		alignas(64) inline constexpr std::array<Bitboard, 64> kingAttacks = ([]() constexpr {
			std::array<Bitboard, 64> output;

			for (int i = 0; i < 64; ++i) {
				const Bitboard spot = 1ull << i;

				output[i] =
					/* (+1, +1) */ ((spot & ~(RankMask::Rank8 | FileMask::hFile)) << 9) |
					/* (-1, +1) */ ((spot & ~(RankMask::Rank8 | FileMask::aFile)) << 7) |
					/* (-1, -1) */ ((spot & ~(RankMask::Rank1 | FileMask::aFile)) >> 9) |
					/* (+1, -1) */ ((spot & ~(RankMask::Rank1 | FileMask::hFile)) >> 7) |
					/* (+1,  0) */ ((spot & ~FileMask::hFile) << 1) |
					/* (-1,  0) */ ((spot & ~FileMask::aFile) >> 1) |
					/* ( 0, +1) */ ((spot & ~RankMask::Rank8) << 8) |
					/* ( 0, -1) */ ((spot & ~RankMask::Rank1) >> 8);
			}

			return output;
		})();

		alignas(64) inline Bitboard rookAttacks[64 * 4096];
		alignas(64) inline Bitboard bishopAttacks[64 * 512];

		alignas(64) inline constexpr Bitboard rookBlocker[64] = {
			0x000101010101017Eull,
			0x000202020202027Cull,
			0x000404040404047Aull,
			0x0008080808080876ull,
			0x001010101010106Eull,
			0x002020202020205Eull,
			0x004040404040403Eull,
			0x008080808080807Eull,
			0x0001010101017E00ull,
			0x0002020202027C00ull,
			0x0004040404047A00ull,
			0x0008080808087600ull,
			0x0010101010106E00ull,
			0x0020202020205E00ull,
			0x0040404040403E00ull,
			0x0080808080807E00ull,
			0x00010101017E0100ull,
			0x00020202027C0200ull,
			0x00040404047A0400ull,
			0x0008080808760800ull,
			0x00101010106E1000ull,
			0x00202020205E2000ull,
			0x00404040403E4000ull,
			0x00808080807E8000ull,
			0x000101017E010100ull,
			0x000202027C020200ull,
			0x000404047A040400ull,
			0x0008080876080800ull,
			0x001010106E101000ull,
			0x002020205E202000ull,
			0x004040403E404000ull,
			0x008080807E808000ull,
			0x0001017E01010100ull,
			0x0002027C02020200ull,
			0x0004047A04040400ull,
			0x0008087608080800ull,
			0x0010106E10101000ull,
			0x0020205E20202000ull,
			0x0040403E40404000ull,
			0x0080807E80808000ull,
			0x00017E0101010100ull,
			0x00027C0202020200ull,
			0x00047A0404040400ull,
			0x0008760808080800ull,
			0x00106E1010101000ull,
			0x00205E2020202000ull,
			0x00403E4040404000ull,
			0x00807E8080808000ull,
			0x007E010101010100ull,
			0x007C020202020200ull,
			0x007A040404040400ull,
			0x0076080808080800ull,
			0x006E101010101000ull,
			0x005E202020202000ull,
			0x003E404040404000ull,
			0x007E808080808000ull,
			0x7E01010101010100ull,
			0x7C02020202020200ull,
			0x7A04040404040400ull,
			0x7608080808080800ull,
			0x6E10101010101000ull,
			0x5E20202020202000ull,
			0x3E40404040404000ull,
			0x7E80808080808000ull,
		};
	
		alignas(64) inline constexpr Bitboard bishopBlocker[64] = {
			0x0040201008040200ull,
			0x0000402010080400ull,
			0x0000004020100A00ull,
			0x0000000040221400ull,
			0x0000000002442800ull,
			0x0000000204085000ull,
			0x0000020408102000ull,
			0x0002040810204000ull,
			0x0020100804020000ull,
			0x0040201008040000ull,
			0x00004020100A0000ull,
			0x0000004022140000ull,
			0x0000000244280000ull,
			0x0000020408500000ull,
			0x0002040810200000ull,
			0x0004081020400000ull,
			0x0010080402000200ull,
			0x0020100804000400ull,
			0x004020100A000A00ull,
			0x0000402214001400ull,
			0x0000024428002800ull,
			0x0002040850005000ull,
			0x0004081020002000ull,
			0x0008102040004000ull,
			0x0008040200020400ull,
			0x0010080400040800ull,
			0x0020100A000A1000ull,
			0x0040221400142200ull,
			0x0002442800284400ull,
			0x0004085000500800ull,
			0x0008102000201000ull,
			0x0010204000402000ull,
			0x0004020002040800ull,
			0x0008040004081000ull,
			0x00100A000A102000ull,
			0x0022140014224000ull,
			0x0044280028440200ull,
			0x0008500050080400ull,
			0x0010200020100800ull,
			0x0020400040201000ull,
			0x0002000204081000ull,
			0x0004000408102000ull,
			0x000A000A10204000ull,
			0x0014001422400000ull,
			0x0028002844020000ull,
			0x0050005008040200ull,
			0x0020002010080400ull,
			0x0040004020100800ull,
			0x0000020408102000ull,
			0x0000040810204000ull,
			0x00000A1020400000ull,
			0x0000142240000000ull,
			0x0000284402000000ull,
			0x0000500804020000ull,
			0x0000201008040200ull,
			0x0000402010080400ull,
			0x0002040810204000ull,
			0x0004081020400000ull,
			0x000A102040000000ull,
			0x0014224000000000ull,
			0x0028440200000000ull,
			0x0050080402000000ull,
			0x0020100804020000ull,
			0x0040201008040200ull,
		};

		namespace detail {
			inline constexpr Bitboard rookAttack(int from, Bitboard occupied) {
				const Bitboard spot = 1ull << from;
				Bitboard mask = 0ull;

				Bitboard up = spot, down = spot, right = spot, left = spot;
				while (up & ~(RankMask::Rank8 | occupied)) { up <<= 8; mask |= up; };
				while (down & ~(RankMask::Rank1 | occupied)) { down >>= 8; mask |= down; }
				while (right & ~(FileMask::hFile | occupied)) { right <<= 1; mask |= right; }
				while (left & ~(FileMask::aFile | occupied)) { left >>= 1; mask |= left; }
				
				return mask & ~spot;
			}

			inline constexpr Bitboard bishopAttack(int from, Bitboard occupied) {
				const Bitboard spot = 1ull << from;
				Bitboard mask = 0ull;

				Bitboard ur = spot, ul = spot, dl = spot, dr = spot;
				while (ur & ~(RankMask::Rank8 | FileMask::hFile | occupied)) { ur <<= 9; mask |= ur; }
				while (ul & ~(RankMask::Rank8 | FileMask::aFile | occupied)) { ul <<= 7; mask |= ul; }
				while (dl & ~(RankMask::Rank1 | FileMask::aFile | occupied)) { dl >>= 9; mask |= dl; }
				while (dr & ~(RankMask::Rank1 | FileMask::hFile | occupied)) { dr >>= 7; mask |= dr; }

				return mask & ~spot;
			}
		}

		inline bool initialized = false;

		/**
		 * Initialize lookup tables. This can safely be called multiple times.
		 */
		inline void init() noexcept {
			if (initialized) return;
			
			for (size_t i = 0; i < 64; ++i)
				for (size_t j = 0; j < 4096; ++j)
					rookAttacks[i * 4096 + j] = detail::rookAttack(i, _pdep_u64(j, rookBlocker[i]));
			for (size_t i = 0; i < 64; ++i)
				for (size_t j = 0; j < 512; ++j)
					bishopAttacks[i * 512 + j] = detail::bishopAttack(i, _pdep_u64(j, bishopBlocker[i]));
			
			initialized = true;
		}

		CHESS_ALWAYS_INLINE inline constexpr Bitboard bishopAttack(const int from, const Bitboard occupied) noexcept {
			return bishopAttacks[(from << 9) + _pext_u64(occupied, bishopBlocker[from])];
		}

		CHESS_ALWAYS_INLINE inline constexpr Bitboard rookAttack(const int from, const Bitboard occupied) noexcept {
			return rookAttacks[(from << 12) + _pext_u64(occupied, rookBlocker[from])];
		}

		CHESS_ALWAYS_INLINE inline constexpr Bitboard queenAttack(const int from, const Bitboard occupied) noexcept {
			return bishopAttack(from, occupied) | rookAttack(from, occupied);
		}

		CHESS_ALWAYS_INLINE inline constexpr Bitboard knightAttack(const int from) noexcept {
			return knightAttacks[from];
		}

		CHESS_ALWAYS_INLINE inline constexpr Bitboard kingAttack(const int from) noexcept {
			return kingAttacks[from];
		}
	}
}