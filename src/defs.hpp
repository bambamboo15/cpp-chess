/**
 * A fast chess library for C++
 */
#pragma once
#include <cstdint>
#include <type_traits>
#include <iostream>

#ifndef CHESS_PROFILE
#	define CHESS_PROFILE
#endif

#if defined(__GNUC__) || defined(__clang__)
#	define CHESS_ALWAYS_INLINE __attribute__((always_inline))
#else
#	define CHESS_ALWAYS_INLINE
#endif
#define CHESS_NODISCARD [[nodiscard]]

#include <cassert>
#ifdef DEBUG
#	define CHESS_ASSERT(cond) assert(cond)
#else
#	define CHESS_ASSERT(cond)
#endif
#define CHESS_ASSERT_SQUARE(square) CHESS_ASSERT((square) >= 0 && (square) < 64)
#define CHESS_ASSERT_COLOR CHESS_ASSERT(game.turn() == Color)

#define CHESS_U64(number) number ## ull

namespace chess {
	/**
	 * Represents a bitboard. A bitboard is a 64-bit unsigned integer, aligning
	 * with the 64 squares of the chessboard. Refer to the `defs.hpp` file for
	 * a graphical representation of this.
	 * 
	 * \cond Here is a figure of how the bits in a bitboard correspond to the
	 *       squares on a chessboard:
	 * 
	 *                         56 57 58 59 60 61 62 63
	 *                         48 49 50 51 52 53 54 55
	 *                         40 41 42 43 44 45 46 47
	 *                         32 33 34 35 36 37 38 49
	 *                         24 25 26 27 28 29 30 31
	 *                         16 17 18 19 20 21 22 23
	 *                         08 09 10 11 12 13 14 15
	 *                         00 01 02 03 04 05 06 07
	 */
	typedef uint64_t Bitboard;
	
	struct DebugBitboard {
		Bitboard bitboard;
	};

	inline constexpr DebugBitboard debugBitboard(Bitboard bitboard) {
		return { bitboard };
	}
	
	inline std::ostream& operator<<(std::ostream& os, const DebugBitboard& debugBitboard) {
		for (int row = 7; row >= 0; --row) {
			for (int col = 0; col < 8; ++col) {
				if (debugBitboard.bitboard & (1ull << (row * 8 + col))) {
					os << "\033[48;2;255;50;50m";
				} else {
					if ((row + col) % 2 == 0) { // light square
					    os << "\033[48;2;205;133;63m";
					} else { // dark square
					    os << "\033[48;2;139;69;19m";
					}
				}
				os << "  \033[0m";
			}
			os << '\n';
		}
		return os;
	}

	/**
	 * Represents a color.
	 */
	enum class Color : uint8_t {
		White = 0,
		Black = 1
	};

	/**
	 * Represents a piece type.
	 */
	enum class PieceType : uint8_t {
		Pawn = 0,
		Knight = 1,
		Bishop = 2,
		Rook = 3,
		Queen = 4,
		King = 5,

		// If you are a user, ignore this
		NoPromotion = 0
	};

	/**
	 * Represents a piece, with color information.
	 */
	enum class Piece : uint8_t {
		WhitePawn = 0, WhiteKnight = 1, WhiteBishop = 2, WhiteRook = 3, WhiteQueen = 4, WhiteKing = 5,
		BlackPawn = 8, BlackKnight = 9, BlackBishop = 10, BlackRook = 11, BlackQueen = 12, BlackKing = 13,
		None
	};

	/**
	 * Gets the color of a Piece.
	 */
	inline constexpr Color getPieceColor(const Piece piece) noexcept {
		return Color(uint8_t(piece) >> 3);
	}

	/**
	 * Gets the PieceType of a Piece.
	 */
	inline constexpr PieceType getPieceType(const Piece piece) noexcept {
		return PieceType(uint8_t(piece) & 7);
	}

	/**
	 * Forms a Piece from a PieceType and Color.
	 */
	inline constexpr Piece makePiece(const PieceType pieceType, const Color color) noexcept {
		return Piece(uint8_t(pieceType) | (uint8_t(color) << 3));
	}

	/**
	 * Provides easy values for squares.
	 */
	namespace Square {
		enum __Square : int {
			A1, B1, C1, D1, E1, F1, G1, H1,
			A2, B2, C2, D2, E2, F2, G2, H2,
			A3, B3, C3, D3, E3, F3, G3, H3,
			A4, B4, C4, D4, E4, F4, G4, H4,
			A5, B5, C5, D5, E5, F5, G5, H5,
			A6, B6, C6, D6, E6, F6, G6, H6,
			A7, B7, C7, D7, E7, F7, G7, H7,
			A8, B8, C8, D8, E8, F8, G8, H8,
			None
		};
	}

	/**
	 * Obtains the file number of the square.
	 */
	inline constexpr int fileOf(const int square) noexcept {
		return square & 7;
	}

	/**
	 * Obtains the rank number of the square.
	 */
	inline constexpr int rankOf(const int square) noexcept {
		return square >> 3;
	}

	namespace Square {
		// GUARANTEE: Square values can be casted to uint8_t and back without loss of information
		static_assert(static_cast<std::underlying_type_t<__Square>>(static_cast<uint8_t>(__Square::None)) == __Square::None);
	}
	
	struct SquareNameInfo {
		char letter;
		char number;
	};

	inline constexpr SquareNameInfo getSquareName(int square) {
		return { static_cast<char>('a' + fileOf(square)), static_cast<char>('1' + rankOf(square)) };
	}
	
	inline std::ostream& operator<<(std::ostream& os, const SquareNameInfo& squareName) {
		return os << squareName.letter << squareName.number;
	}

	/**
	 * Provides easy values for files.
	 */
	namespace File {
		enum __File : int {
			aFile, bFile, cFile, dFile, eFile, fFile, gFile, hFile
		};
	};

	/**
	 * Provides easy masks for files.
	 */
	namespace FileMask {
		inline constexpr Bitboard aFile = 0x01'01'01'01'01'01'01'01ull;
		inline constexpr Bitboard bFile = 0x02'02'02'02'02'02'02'02ull;
		inline constexpr Bitboard cFile = 0x04'04'04'04'04'04'04'04ull;
		inline constexpr Bitboard dFile = 0x08'08'08'08'08'08'08'08ull;
		inline constexpr Bitboard eFile = 0x10'10'10'10'10'10'10'10ull;
		inline constexpr Bitboard fFile = 0x20'20'20'20'20'20'20'20ull;
		inline constexpr Bitboard gFile = 0x40'40'40'40'40'40'40'40ull;
		inline constexpr Bitboard hFile = 0x80'80'80'80'80'80'80'80ull;
	};

	/**
	 * Provides easy values for ranks.
	 */
	namespace Rank {
		enum __Rank : int {
			Rank1, Rank2, Rank3, Rank4, Rank5, Rank6, Rank7, Rank8
		};
	};

	/**
	 * Provides easy masks for ranks.
	 */
	namespace RankMask {
		inline constexpr Bitboard Rank1 = 0x00'00'00'00'00'00'00'FFull;
		inline constexpr Bitboard Rank2 = 0x00'00'00'00'00'00'FF'00ull;
		inline constexpr Bitboard Rank3 = 0x00'00'00'00'00'FF'00'00ull;
		inline constexpr Bitboard Rank4 = 0x00'00'00'00'FF'00'00'00ull;
		inline constexpr Bitboard Rank5 = 0x00'00'00'FF'00'00'00'00ull;
		inline constexpr Bitboard Rank6 = 0x00'00'FF'00'00'00'00'00ull;
		inline constexpr Bitboard Rank7 = 0x00'FF'00'00'00'00'00'00ull;
		inline constexpr Bitboard Rank8 = 0xFF'00'00'00'00'00'00'00ull;
	};

	/**
	 * Represents move flags.
	 */
	enum class MoveFlags : int {
		QuietMove = 0b0000,
		DoublePawnPush = 0b0001,
		KingCastle = 0b0010,
		QueenCastle = 0b0011,
		Capture = 0b0100,
		EnPassantCapture = 0b0101,
		KnightPromotion = 0b1000,
		BishopPromotion = 0b1001,
		RookPromotion = 0b1010,
		QueenPromotion = 0b1011,
		KnightPromotionCapture = 0b1100,
		BishopPromotionCapture = 0b1101,
		RookPromotionCapture = 0b1110,
		QueenPromotionCapture = 0b1111,
		Zero = 0b0000
	};

	/**
	 * Represents castling rights.
	 */
	enum class CastlingFlags : uint8_t {
		None = 0b0000,
		WhiteKingside = 0b0001,
		WhiteQueenside = 0b0010,
		BlackKingside = 0b0100,
		BlackQueenside = 0b1000
	};

	/**
	 * ORs two CastlingFlags.
	 */
	inline constexpr CastlingFlags operator|(CastlingFlags lhs, CastlingFlags rhs) {
		return CastlingFlags(uint8_t(lhs) | uint8_t(rhs));
	}
	
	inline constexpr CastlingFlags& operator|=(CastlingFlags& lhs, CastlingFlags rhs) {
		return lhs = lhs | rhs;
	}

	/**
	 * ANDs two CastlingFlags.
	 */
	inline constexpr CastlingFlags operator&(CastlingFlags lhs, CastlingFlags rhs) {
		return CastlingFlags(uint8_t(lhs) & uint8_t(rhs));
	}
	
	inline constexpr CastlingFlags& operator&=(CastlingFlags& lhs, CastlingFlags rhs) {
		return lhs = lhs & rhs;
	}

	/**
	 * NOTs a CastlingFlags. Note that this does an additional masking operations to keep the
	 * upper four bits of the underlying type clean.
	 */
	inline constexpr CastlingFlags operator~(CastlingFlags flag) {
		return CastlingFlags((~uint8_t(flag)) & uint8_t(0b00001111));
	}

	/**
	 * Swaps a color.
	 */
	inline constexpr Color operator~(Color color) noexcept {
		return color == Color::White ? Color::Black : Color::White;
	}

	/**
	 * The move generation mode.
	 */
	enum class MoveGenerationMode {
		Legal,
		PseudoLegal
	};

	/**
	 * Example FEN strings for you to use.
	 */
	namespace QuickFEN {
		inline constexpr const char* start = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
		inline constexpr const char* kiwipete = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
		inline constexpr const char* tricky = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1";
		inline constexpr const char* complex = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
		inline constexpr const char* buggy = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
	}
}