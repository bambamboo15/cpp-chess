/**
 * A fast chess library for C++
 */
#pragma once
#include "defs.hpp"

namespace chess {
	template <Color Color>
	CHESS_ALWAYS_INLINE inline constexpr Bitboard forward(Bitboard bitboard) noexcept {
		if constexpr (Color == Color::White) return bitboard << 8;
		return bitboard >> 8;
	}

	template <Color Color>
	CHESS_ALWAYS_INLINE inline constexpr int forwardSquare(int square) noexcept {
		if constexpr (Color == Color::White) return square + 8;
		return square - 8;
	}

	template <Color Color>
	CHESS_ALWAYS_INLINE inline constexpr Bitboard doubleForward(Bitboard bitboard) noexcept {
		if constexpr (Color == Color::White) return bitboard << 16;
		return bitboard >> 16;
	}

	template <Color Color>
	CHESS_ALWAYS_INLINE inline constexpr int doubleForwardSquare(int square) noexcept {
		if constexpr (Color == Color::White) return square + 16;
		return square - 16;
	}

	template <Color Color>
	inline consteval Bitboard pawnStartingRank() noexcept {
		if constexpr (Color == Color::White) return RankMask::Rank2;
		return RankMask::Rank7;
	}

	template <Color Color>
	inline consteval Bitboard pawnLastRank() noexcept {
		if constexpr (Color == Color::White) return RankMask::Rank7;
		return RankMask::Rank2;
	}

	template <Color Color>
	inline consteval Bitboard pawnEnPassantRank() noexcept {
		if constexpr (Color == Color::White) return RankMask::Rank5;
		return RankMask::Rank4;
	}

	CHESS_ALWAYS_INLINE inline constexpr int toSquare(const Bitboard bitboard) noexcept {
		return __builtin_ctzll(bitboard);
	}

	CHESS_ALWAYS_INLINE inline constexpr int popcount(const Bitboard bitboard) noexcept {
		return __builtin_popcountll(bitboard);
	}

	CHESS_ALWAYS_INLINE inline constexpr int popLSB(Bitboard& bitboard) noexcept {
		const int index = toSquare(bitboard);
		bitboard &= bitboard - 1;
		return index;
	}

	template <Color Color>
	inline consteval CastlingFlags kingsideCastleFlag() noexcept {
		if constexpr (Color == Color::White) return CastlingFlags::WhiteKingside;
		return CastlingFlags::BlackKingside;
	}

	template <Color Color>
	inline consteval CastlingFlags queensideCastleFlag() noexcept {
		if constexpr (Color == Color::White) return CastlingFlags::WhiteQueenside;
		return CastlingFlags::BlackQueenside;
	}

	template <Color Color>
	inline consteval int kingsideCastleRookFromSquare() noexcept {
		if constexpr (Color == Color::White) return Square::H1;
		return Square::H8;
	}

	template <Color Color>
	inline consteval int queensideCastleRookFromSquare() noexcept {
		if constexpr (Color == Color::White) return Square::A1;
		return Square::A8;
	}

	template <Color Color>
	inline consteval int kingsideCastleRookToSquare() noexcept {
		if constexpr (Color == Color::White) return Square::F1;
		return Square::F8;
	}

	template <Color Color>
	inline consteval int queensideCastleRookToSquare() noexcept {
		if constexpr (Color == Color::White) return Square::D1;
		return Square::D8;
	}

	template <Color Color>
	inline consteval int initialKingSquare() noexcept {
		if constexpr (Color == Color::White) return Square::E1;
		return Square::E8;
	}

	template <Color Color>
	inline consteval int kingsideCastleKingToSquare() noexcept {
		if constexpr (Color == Color::White) return Square::G1;
		return Square::G8;
	}

	template <Color Color>
	inline consteval int queensideCastleKingToSquare() noexcept {
		if constexpr (Color == Color::White) return Square::C1;
		return Square::C8;
	}

	// The two squares must not be equal!
	CHESS_ALWAYS_INLINE inline constexpr Bitboard squaresBetween(int greater, int lesser) noexcept {
		return (1ull << greater) - (2ull << lesser);
	}

	// The two squares must not be equal!
	CHESS_ALWAYS_INLINE inline constexpr Bitboard squaresBetweenUnordered(int a, int b) noexcept {
		return a > b ? squaresBetween(a, b) : squaresBetween(b, a);
	}
}