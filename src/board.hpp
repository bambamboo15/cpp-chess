/**
 * A fast chess library for C++
 */
#pragma once
#include "defs.hpp"

namespace chess {
	class Board {
		// The bitboards, in order, are:
		//   - (wP) White pawn
		//   - (wN) White knight
		//   - (wB) White bishop
		//   - (wR) White rook
		//   - (wQ) White queen
		//   - (wK) White king
		//   -      UNUSED BITBOARD FOR PADDING
		//   -      UNUSED BITBOARD FOR PADDING
		//   - (bP) Black pawn
		//   - (bN) Black knight
		//   - (bB) Black bishop
		//   - (bR) Black rook
		//   - (bQ) Black queen
		//   - (bK) Black king
		//   -      WHITE
		//   -      BLACK
		//   -      OCCUPIED
		Bitboard bitboards[14];
		Bitboard m_colorOccupancy[2];
		Bitboard m_occupied;

		// Mailbox representation of board -- this is for more efficiency in computations
		Piece mailbox[64];

	public:
		constexpr Board() : Board(
			0x00'00'00'00'00'00'00'00ull,
			0x00'00'00'00'00'00'00'00ull,
			0x00'00'00'00'00'00'00'00ull,
			0x00'00'00'00'00'00'00'00ull,
			0x00'00'00'00'00'00'00'00ull,
			0x00'00'00'00'00'00'00'00ull,
			0x00'00'00'00'00'00'00'00ull,
			0x00'00'00'00'00'00'00'00ull,
			0x00'00'00'00'00'00'00'00ull,
			0x00'00'00'00'00'00'00'00ull,
			0x00'00'00'00'00'00'00'00ull,
			0x00'00'00'00'00'00'00'00ull
		)
		{ }

		constexpr Board(Bitboard wp, Bitboard wn, Bitboard wb,
		                Bitboard wr, Bitboard wq, Bitboard wk,
		                Bitboard bp, Bitboard bn, Bitboard bb,
		                Bitboard br, Bitboard bq, Bitboard bk) :
			bitboards{wp, wn, wb, wr, wq, wk, 0, 0, bp, bn, bb, br, bq, bk},
			m_colorOccupancy{wp | wn | wb | wr | wq | wk, bp | bn | bb | br | bq | bk},
			m_occupied{wp | wn | wb | wr | wq | wk | bp | bn | bb | br | bq | bk}
		{
			for (size_t i = 0; i < 64; ++i)
				mailbox[i] = Piece::None;
			
			// Assumes that order of bitboards is same as order of constants in Piece enumeration
			for (size_t b = 0; b < 14; ++b)
				for (size_t i = 0; i < 64; ++i)
					if (bitboards[b] & (1ull << i))
						mailbox[i] = static_cast<Piece>(b);
		}

		template <Color Color>
		CHESS_ALWAYS_INLINE inline constexpr Bitboard pawns() const noexcept {
			if constexpr (Color == Color::White) return bitboards[0];
			return bitboards[8];
		}

		template <Color Color>
		CHESS_ALWAYS_INLINE inline constexpr Bitboard knights() const noexcept {
			if constexpr (Color == Color::White) return bitboards[1];
			return bitboards[9];
		}

		template <Color Color>
		CHESS_ALWAYS_INLINE inline constexpr Bitboard bishops() const noexcept {
			if constexpr (Color == Color::White) return bitboards[2];
			return bitboards[10];
		}

		template <Color Color>
		CHESS_ALWAYS_INLINE inline constexpr Bitboard rooks() const noexcept {
			if constexpr (Color == Color::White) return bitboards[3];
			return bitboards[11];
		}

		template <Color Color>
		CHESS_ALWAYS_INLINE inline constexpr Bitboard queens() const noexcept {
			if constexpr (Color == Color::White) return bitboards[4];
			return bitboards[12];
		}

		template <Color Color>
		CHESS_ALWAYS_INLINE inline constexpr Bitboard kings() const noexcept {
			if constexpr (Color == Color::White) return bitboards[5];
			return bitboards[13];
		}

		template <Color Color>
		CHESS_ALWAYS_INLINE inline constexpr Bitboard occupancy() const noexcept {
			if constexpr (Color == Color::White) return m_colorOccupancy[0];
			return m_colorOccupancy[1];
		}

		CHESS_ALWAYS_INLINE inline constexpr Bitboard occupied() const noexcept {
			return m_occupied;
		}

		CHESS_ALWAYS_INLINE inline constexpr Piece pieceAt(int square) const noexcept {
			return mailbox[square];
		}

		CHESS_ALWAYS_INLINE inline constexpr bool isSquareOccupied(int square) const noexcept {
			return mailbox[square] == Piece::None;
		}

		CHESS_ALWAYS_INLINE inline constexpr Bitboard pieceBitboard(Piece piece) const noexcept {
			// Assumes that order of bitboards is same as order of constants in Piece enumeration
			return bitboards[static_cast<size_t>(piece)];
		}

		/**
		 * @tparam Color The compile-time color of the piece, for optimization purposes.
		 * @param piece The piece to place.
		 * @param square The square to place the piece on.
		 * 
		 * Places a piece on a given square. Assumes that the destination square is empty, and
		 * that the arguments are valid.
		 */
		CHESS_ALWAYS_INLINE inline constexpr void putPiece(Piece piece, int square) noexcept {
			CHESS_ASSERT_SQUARE(square);
			CHESS_ASSERT(mailbox[square] == Piece::None);
			CHESS_ASSERT(piece != Piece::None);

			const Bitboard mask = 1ull << square;

			bitboards[static_cast<size_t>(piece)] |= mask;
			m_occupied |= mask;
			m_colorOccupancy[static_cast<size_t>(getPieceColor(piece))] |= mask;
			
			mailbox[square] = piece;
		}

		/**
		 * @param square The square to remove the piece from.
		 * 
		 * Removes a piece from a given square. Assumes that the square is occupied, and
		 * that the arguments are valid.
		 */
		CHESS_ALWAYS_INLINE inline constexpr void removePiece(int square) noexcept {
			CHESS_ASSERT_SQUARE(square);
			CHESS_ASSERT(mailbox[square] != Piece::None);

			const Piece piece = mailbox[square];
			const Bitboard mask = 1ull << square;

			bitboards[static_cast<size_t>(piece)] ^= mask;
			m_occupied ^= mask;
			m_colorOccupancy[static_cast<size_t>(getPieceColor(piece))] ^= mask;
			
			mailbox[square] = Piece::None;
		}

		/**
		 * @param from The square to move from.
		 * @param to The square to move to.
		 * 
		 * Moves a piece from a given square to another given square, assuming that the destination square is empty,
		 * and the starting square is occupied. This also means that the provided squares should be valid and nonequal.
		 */
		CHESS_ALWAYS_INLINE inline constexpr void movePiece(int from, int to) noexcept {
			CHESS_ASSERT_SQUARE(from);
			CHESS_ASSERT_SQUARE(to);
			CHESS_ASSERT(mailbox[from] != Piece::None);
			CHESS_ASSERT(mailbox[to] == Piece::None);

			const Piece piece = mailbox[from];
			const Bitboard mask = (1ull << from) | (1ull << to);

			bitboards[static_cast<size_t>(piece)] ^= mask;
			m_occupied ^= mask;
			m_colorOccupancy[static_cast<size_t>(getPieceColor(piece))] ^= mask;
			
			mailbox[to] = piece;
			mailbox[from] = Piece::None;
		}
	};
	
	inline std::ostream& operator<<(std::ostream& os, const Board& board) {
		for (int row = 7; row >= 0; --row) {
			os
				<< "  +---+---+---+---+---+---+---+---+\n"
				<< (row + 1) << ' ';

			for (int col = 0; col < 8; ++col) {
				os << "| ";

				switch (board.pieceAt(col + row * 8)) {
					case Piece::None:			os << ' '; break;
					case Piece::WhitePawn:		os << 'P'; break;
					case Piece::WhiteKnight:	os << 'N'; break;
					case Piece::WhiteBishop:	os << 'B'; break;
					case Piece::WhiteRook:		os << 'R'; break;
					case Piece::WhiteQueen:		os << 'Q'; break;
					case Piece::WhiteKing:		os << 'K'; break;
					case Piece::BlackPawn:		os << 'p'; break;
					case Piece::BlackKnight:	os << 'n'; break;
					case Piece::BlackBishop:	os << 'b'; break;
					case Piece::BlackRook:		os << 'r'; break;
					case Piece::BlackQueen:		os << 'q'; break;
					case Piece::BlackKing:		os << 'k'; break;
					default:					os << '?'; break;
				}

				os << ' ';
			}
			os << "|\n";
		}

		return os
			<< "  +---+---+---+---+---+---+---+---+\n"
			<< "    a   b   c   d   e   f   g   h  \n";
	}
}