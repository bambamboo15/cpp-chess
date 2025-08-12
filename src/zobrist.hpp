/**
 * A fast chess library for C++
 */
#pragma once
#include "defs.hpp"
#include <array>

namespace chess {
	/**
	 * A fast 64-bit pseudorandom number generator straight from Stockfish. You can use this!
	 * 
	 * https://github.com/official-stockfish/Stockfish/blob/master/src/misc.h
	 */
	class PRNG {
		uint64_t seed;
		
		public:
			explicit constexpr PRNG(uint64_t seed) : seed{seed} {
				CHESS_ASSERT(seed);
			}

			inline constexpr uint64_t rand64() noexcept {
				seed ^= seed >> 12;
				seed ^= seed << 25;
				seed ^= seed >> 27;
				return seed * 2685821657736338717LL;
			}

			inline constexpr uint64_t sparseRand64() noexcept {
				return rand64() & rand64() & rand64();
			}
	};

	namespace zobrist {
		typedef uint64_t Key;
		
		inline uint64_t pieceSquareTable[16][64];
		inline uint64_t enPassantTable[8];
		inline uint64_t castlingTable[16];
		inline uint64_t side, noPawns;
		inline bool initialized = false;

		/**
		 * Precompute Zobrist-related global information. This can safely be called multiple times.
		 */
		inline constexpr void init() noexcept {
			if (initialized) return;

			PRNG rng(1070372);

			for (Piece piece : {
				Piece::WhitePawn, Piece::WhiteKnight, Piece::WhiteBishop,
				Piece::WhiteRook, Piece::WhiteQueen, Piece::WhiteKing,
				Piece::BlackPawn, Piece::BlackKnight, Piece::BlackBishop,
				Piece::BlackRook, Piece::BlackQueen, Piece::BlackKing
			})
				for (int square = Square::A1; square <= Square::H8; ++square)
					pieceSquareTable[(size_t)piece][square] = rng.rand64();
			
			for (int file = 0; file < 8; ++file)
				enPassantTable[file] = rng.rand64();
			
			for (int cr = 0; cr < 16; ++cr)
				castlingTable[cr] = rng.rand64();
			
			side = rng.rand64();
			noPawns = rng.rand64();

			initialized = true;
		}
	}
}