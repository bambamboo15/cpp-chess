/**
 * A fast chess library for C++
 */
#pragma once
#include "utils.hpp"

namespace chess {
	// Move representation from https://www.chessprogramming.org/Encoding_Moves
	class Move {
		uint16_t m_data;

	public:
		constexpr Move() : m_data{0} { }

		constexpr Move(int from, int to, MoveFlags flags) :
			m_data{ uint16_t((uint16_t(flags) << 12) | (from << 6) | to) }
		{ }
		
		inline constexpr int getTo() const noexcept { return m_data & 0x3F; }
		inline constexpr int getFrom() const noexcept { return (m_data >> 6) & 0x3F; }
		inline constexpr MoveFlags getFlags() const noexcept { return MoveFlags(m_data >> 12); }

		inline constexpr void setTo(int to) noexcept { m_data &= ~0x3F; m_data |= to & 0x3F; }
		inline constexpr void setFrom(int from) noexcept { m_data &= ~0xFC0; m_data |= (from & 0x3F) << 6; }
		inline constexpr void setFlags(MoveFlags flags) noexcept { m_data &= ~0x3F000; m_data |= (uint16_t(flags) & 0x0F) << 12; }

		inline constexpr bool isCapture() const noexcept { return (m_data & (0b0100 << 12)) != 0; }
		inline constexpr bool isQuietMove() const noexcept { return (m_data & (0b1111 << 12)) == (0b0000 << 12); }
		inline constexpr bool isDoublePawnPush() const noexcept { return (m_data & (0b1111 << 12)) == (0b0001 << 12); }
		inline constexpr bool isEnPassant() const noexcept { return (m_data & (0b1111 << 12)) == (0b0101 << 12); }
		inline constexpr bool isKingsideCastle() const noexcept { return (m_data & (0b1111 << 12)) == (0b0010 << 12); }
		inline constexpr bool isQueensideCastle() const noexcept { return (m_data & (0b1111 << 12)) == (0b0011 << 12); }
		inline constexpr bool isCastle() const noexcept { return (m_data & (0b111 << 13)) == (0b001 << 13); }
		inline constexpr bool isPromotion() const noexcept { return (m_data & (0b1 << 15)) == (0b1 << 15); }
		inline constexpr bool isKnightPromotion() const noexcept { return (m_data & (0b1011 << 12)) == (0b1000 << 12); }
		inline constexpr bool isBishopPromotion() const noexcept { return (m_data & (0b1011 << 12)) == (0b1001 << 12); }
		inline constexpr bool isRookPromotion() const noexcept { return (m_data & (0b1011 << 12)) == (0b1010 << 12); }
		inline constexpr bool isQueenPromotion() const noexcept { return (m_data & (0b1011 << 12)) == (0b1011 << 12); }

		inline constexpr uint16_t data() const noexcept { return m_data; }
		inline constexpr bool isNull() const noexcept { return m_data == 0; }

		static consteval Move null() { return {}; }

		template <Color Color>
		inline constexpr int captureDestinationSquare() const noexcept {
			constexpr int offset = Color == Color::White ? -8 : 8;
			return getTo() + (isEnPassant() ? offset : 0);
		}

		template <Color Color>
		inline constexpr Piece capturedPiece(const Piece pieceDest) const noexcept {
			constexpr Piece epCapturedPiece = Color == Color::White ? Piece::BlackPawn : Piece::WhitePawn;
			return isEnPassant() ? epCapturedPiece : pieceDest;
		}

		template <Color Color>
		inline constexpr int doublePawnPushEnPassantSquare() const noexcept {
			constexpr int offset = Color == Color::White ? 8 : -8;
			return getFrom() + offset;
		}
		
		inline constexpr PieceType promotionPieceType() const noexcept {
			return PieceType(((m_data >> 12) & 0b0011) + 0b0001);
		}
		
		template <Color Color>
		inline constexpr Piece promotionPiece() const noexcept {
			return makePiece(promotionPieceType(), Color);
		}
	};

	inline constexpr bool operator==(const Move lhs, const Move rhs) {
		return lhs.data() == rhs.data();
	}

	// UCI-compatible move representation
	inline std::ostream& operator<<(std::ostream& os, const Move& move) {
		os
			<< getSquareName(move.getFrom())
			<< getSquareName(move.getTo());
		
		if (move.isPromotion()) {
			switch (move.promotionPieceType()) {
				case PieceType::Knight: return os << 'n';
				case PieceType::Bishop: return os << 'b';
				case PieceType::Rook: return os << 'r';
				case PieceType::Queen: return os << 'q';
			}
		}

		return os;
	}
}