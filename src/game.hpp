/**
 * A fast chess library for C++
 */
#pragma once
#include "board.hpp"
#include "move.hpp"
#include "lookup.hpp"
#include "zobrist.hpp"

namespace chess {
	struct UndoInfo {
		int halfMoveCounter;
		Piece capturedPiece;
		CastlingFlags castlingRights;
		uint8_t enPassantSquare; // uint8_t for memory purposes
	};
	static_assert(sizeof(UndoInfo) <= sizeof(void*));

	/**
	 * This represents a single instance of a chess game. This is a very heavy object.
	 * 
	 * \warning This can hold only up to 512 moves!
	 */
	class Game {
		Board m_board;						/* the internal board position */
		Color m_turn;						/* the current player to move */

		zobrist::Key m_history[512];		/* hashes from first position to last */
		zobrist::Key m_hash;				/* zobrist hash */

		CastlingFlags m_castlingRights;		/* castling rights */
		int m_enPassantSquare;				/* en-passant square, -1 if none */
		int m_halfMoveCounter;				/* half-move counter */
		int m_ply;							/* number of ply */

	private:
		/**
		 * Sets up incremental state, that is, state that is initialized once and updated
		 * incrementally as moves are made.
		 */
		inline constexpr void setupIncrementalState() noexcept {
			m_hash = 0;

			for (Bitboard b = m_board.occupied(); b;) {
				const int square = popLSB(b);
				const Piece piece = m_board.pieceAt(square);

				m_hash ^= zobrist::pieceSquareTable[(size_t)piece][square];
			}

			if (m_turn == Color::Black)
				m_hash ^= zobrist::side;
			
			if (m_enPassantSquare != Square::None)
				m_hash ^= zobrist::enPassantTable[fileOf(m_enPassantSquare)];
			
			m_hash ^= zobrist::castlingTable[(size_t)m_castlingRights];

			m_history[m_ply] = m_hash;
		}
	
	public:
		constexpr Game() : Game(QuickFEN::start) { }
		Game(const Game&) = delete;
		Game(Game&&) = delete;
		Game& operator=(const Game&) = delete;
		Game& operator=(Game&&) = delete;

		// FEN must be valid.
		explicit constexpr Game(const std::string_view fen) {
			lookup::init();
			zobrist::init();

			init(fen);
		}

		// Initialize the Game with a given FEN. By default, this is the default position.
		inline constexpr void init(const std::string_view fen = QuickFEN::start) {
			// Initialize default
			m_board = Board{};
			m_turn = Color::White;
			m_castlingRights = CastlingFlags::None;
			m_enPassantSquare = Square::None;
			m_halfMoveCounter = 0;
			m_ply = 0;

			const char* chr = fen.begin();
			const char* const end = fen.end();

			// 1. Piece placement
			for (int row = 7, col = 0; *chr != ' '; ++chr) {
				const int square = col + row * 8;

				switch (*chr) {
					case '1': col += 1; break;
					case '2': col += 2; break;
					case '3': col += 3; break;
					case '4': col += 4; break;
					case '5': col += 5; break;
					case '6': col += 6; break;
					case '7': col += 7; break;
					case '8': col += 8; break;

					case '/': col = 0; --row; break;

					case 'P': ++col; m_board.putPiece(Piece::WhitePawn, square); break;
					case 'N': ++col; m_board.putPiece(Piece::WhiteKnight, square); break;
					case 'B': ++col; m_board.putPiece(Piece::WhiteBishop, square); break;
					case 'R': ++col; m_board.putPiece(Piece::WhiteRook, square); break;
					case 'Q': ++col; m_board.putPiece(Piece::WhiteQueen, square); break;
					case 'K': ++col; m_board.putPiece(Piece::WhiteKing, square); break;
					case 'p': ++col; m_board.putPiece(Piece::BlackPawn, square); break;
					case 'n': ++col; m_board.putPiece(Piece::BlackKnight, square); break;
					case 'b': ++col; m_board.putPiece(Piece::BlackBishop, square); break;
					case 'r': ++col; m_board.putPiece(Piece::BlackRook, square); break;
					case 'q': ++col; m_board.putPiece(Piece::BlackQueen, square); break;
					case 'k': ++col; m_board.putPiece(Piece::BlackKing, square); break;
				}
			}
			++chr;

			// 2. Active color
			m_turn = *chr == 'w' ? Color::White : Color::Black;
			++chr;
			++chr;

			// 3. Castling availability
			while (*chr != ' ') {
				switch (*chr) {
					case 'K': m_castlingRights |= CastlingFlags::WhiteKingside; break;
					case 'Q': m_castlingRights |= CastlingFlags::WhiteQueenside; break;
					case 'k': m_castlingRights |= CastlingFlags::BlackKingside; break;
					case 'q': m_castlingRights |= CastlingFlags::BlackQueenside; break;
				}
				++chr;
			}
			++chr;

			// 4. En-passant square.
			if (*chr == '-') {
				++chr;
				++chr;
			} else {
				m_enPassantSquare = ((*chr++) - 'a');
				m_enPassantSquare += ((*chr++) - '1') * 8;
				++chr;
			}

			// 5. Half-move clock.
			for (; *chr != ' '; ++chr) {
				m_halfMoveCounter *= 10;
				m_halfMoveCounter += *chr - '0';
			}
			++chr;

			// 6. Full-move counter (in this case, converted to ply)
			int fm = 0;
			for (; *chr != ' ' && chr != end; ++chr) {
				fm *= 10;
				fm += *chr - '0';
			}
			m_ply = fm * 2 + static_cast<int>(m_turn);

			// Set up incremental state
			setupIncrementalState();
		}

		inline constexpr Color turn() const noexcept { return m_turn; }
		inline constexpr const Board& board() const noexcept { return m_board; }
		inline constexpr CastlingFlags castlingRights() const noexcept { return m_castlingRights; }
		inline constexpr int enPassantSquare() const noexcept { return m_enPassantSquare; }
		inline constexpr int halfMoveCounter() const noexcept { return m_halfMoveCounter; }
		inline constexpr int fullMoveCount() const noexcept { return m_ply >> 1; }
		inline constexpr int ply() const noexcept { return m_ply; }
		inline constexpr zobrist::Key zobristHash() const noexcept { return m_history[ply()]; }

		/**
		 * Has the game ended in 50-move rule?
		 * 
		 * \note If the drawing move delivers checkmate, the game ends in checkmate. Check that before this.
		 */
		inline constexpr bool draw50MoveRule() const noexcept {
			return halfMoveCounter() > 99;
		}

		/**
		 * Has the game just ended in threefold repetition?
		 * 
		 * \warning This must be called right after the threefold repetition move was played!
		 */
		inline constexpr bool drawThreefoldRepetition() const noexcept {
			if (m_ply < 8)
				return false;
			
			const auto lastHash = zobristHash();
			const int lastCandidate = m_ply - halfMoveCounter();

			int times = 0;
			for (int i = m_ply; i >= lastCandidate; i -= 2)
				times += m_history[i] == lastHash;

			return times >= 3;
		}

		/**
		 * Makes a move without checking for anything.
		 */
		template <Color Color>
		inline constexpr UndoInfo make(const Move move) noexcept {
			CHESS_PROFILE;
			
			CHESS_ASSERT(m_turn == Color);

			const int from = move.getFrom();
			const int to = move.getTo();
			const Piece pieceFrom = m_board.pieceAt(from);
			const Piece pieceTo = m_board.pieceAt(to);

			const UndoInfo undoInfo = {
				m_halfMoveCounter,
				move.capturedPiece<Color>(pieceTo),
				m_castlingRights,
				static_cast<uint8_t>(m_enPassantSquare)
			};

			// increment half-move-clock and ply, but reset half-move clock if pawn move or capture
			++m_halfMoveCounter, ++m_ply;

			[[unlikely]]
			if (pieceFrom == makePiece(PieceType::Pawn, Color) || move.isCapture())
				m_halfMoveCounter = 0;

			// switch side to move, and adjust turn hash
			m_turn = ~Color;
			m_hash ^= zobrist::side;
			
			// remove en-passant data from previous position (if any)
			if (m_enPassantSquare != Square::None)
				m_hash ^= zobrist::enPassantTable[fileOf(m_enPassantSquare)];
			
			// adjust en-passant square according to if move was double pawn-push or not
			m_enPassantSquare = move.isDoublePawnPush()
				? move.doublePawnPushEnPassantSquare<Color>()
				: Square::None;
			
			// include en-passant data in hash (if any)
			if (m_enPassantSquare != Square::None)
				m_hash ^= zobrist::enPassantTable[fileOf(m_enPassantSquare)];
			
			// TODO: Zobrist piece-square table
			
			// If any of the following happens relevant to castling:
			//   1) If castling, both castling sides are invalidated (this is also covered by rule #2)
			//   2) If the king moves, both castling sides are invalidated
			//   3) If a rook moves from a rook castling square, that side is invalidated
			//   4) If a rook is captured from a rook castling square, that side (OF OPPOSITE COLOR) is invalidated
			m_hash ^= zobrist::castlingTable[(size_t)m_castlingRights];

			if (pieceFrom == makePiece(PieceType::King, Color)) {
				m_castlingRights &= ~(kingsideCastleFlag<Color>() | queensideCastleFlag<Color>());
			}
			
			else if (pieceFrom == makePiece(PieceType::Rook, Color)) {
				if (from == kingsideCastleRookFromSquare<Color>()) m_castlingRights &= ~kingsideCastleFlag<Color>();
				else if (from == queensideCastleRookFromSquare<Color>()) m_castlingRights &= ~queensideCastleFlag<Color>();
			}

			if (pieceTo == makePiece(PieceType::Rook, ~Color)) { // for en-passant, pieceTo will just be Piece::None, no worries :)
				if (to == kingsideCastleRookFromSquare<~Color>()) m_castlingRights &= ~kingsideCastleFlag<~Color>();
				else if (to == queensideCastleRookFromSquare<~Color>()) m_castlingRights &= ~queensideCastleFlag<~Color>();
			}

			m_hash ^= zobrist::castlingTable[(size_t)m_castlingRights];

			// Capture
			if (move.isCapture()) {
				const int captureDestSquare = move.captureDestinationSquare<Color>();
				m_hash ^= zobrist::pieceSquareTable[(size_t)m_board.pieceAt(captureDestSquare)][captureDestSquare];
				m_board.removePiece(captureDestSquare);
			}
			
			// Move the piece from the "from" to "to" square, promoted-to piece if promotion
			if (move.isPromotion()) {
				const Piece promoPiece = move.promotionPiece<Color>();

				m_hash ^= zobrist::pieceSquareTable[(size_t)pieceFrom][from];
				m_hash ^= zobrist::pieceSquareTable[(size_t)promoPiece][to];

				m_board.removePiece(from);
				m_board.putPiece(promoPiece, to);
			} else {
				m_hash ^= zobrist::pieceSquareTable[(size_t)pieceFrom][from];
				m_hash ^= zobrist::pieceSquareTable[(size_t)pieceFrom][to];

				m_board.movePiece(from, to);
			}

			// Castling
			if (move.isKingsideCastle()) {
				constexpr int rookFromSquare = kingsideCastleRookFromSquare<Color>();
				constexpr int rookToSquare = kingsideCastleRookToSquare<Color>();
				constexpr Piece rook = makePiece(PieceType::Rook, Color);

				m_hash ^= zobrist::pieceSquareTable[(size_t)rook][rookFromSquare];
				m_hash ^= zobrist::pieceSquareTable[(size_t)rook][rookToSquare];

				m_board.movePiece(rookFromSquare, rookToSquare);
			} else if (move.isQueensideCastle()) {
				constexpr int rookFromSquare = queensideCastleRookFromSquare<Color>();
				constexpr int rookToSquare = queensideCastleRookToSquare<Color>();
				constexpr Piece rook = makePiece(PieceType::Rook, Color);

				m_hash ^= zobrist::pieceSquareTable[(size_t)rook][rookFromSquare];
				m_hash ^= zobrist::pieceSquareTable[(size_t)rook][rookToSquare];

				m_board.movePiece(rookFromSquare, rookToSquare);
			}
			
			// Store hash in threefold repetition table
			m_history[m_ply] = m_hash;

			return undoInfo;
		}

		/**
		 * Unmakes a move without checking for anything.
		 * 
		 * The color template parameter should be the COLOR OPPOSITE CURRENT TURN.
		 */
		template <Color Color>
		inline constexpr void unmake(const Move move, const UndoInfo undoInfo) noexcept {
			CHESS_PROFILE;

			CHESS_ASSERT(m_turn != Color);

			m_castlingRights = undoInfo.castlingRights;
			m_halfMoveCounter = undoInfo.halfMoveCounter;
			m_enPassantSquare = static_cast<int>(undoInfo.enPassantSquare);
			m_turn = Color;

			--m_ply;
			
			const int from = move.getFrom(), to = move.getTo();
			const Piece piece = m_board.pieceAt(to), captured = undoInfo.capturedPiece;
			
			// Micro-operation: Remove piece from "to" square and place it on "from" square
			//                  (if promotion, then just place pawn on "from" square)
			if (move.isPromotion()) {
				m_board.removePiece(move.getTo());
				m_board.putPiece(makePiece(PieceType::Pawn, Color), move.getFrom());
			} else {
				m_board.movePiece(move.getTo(), move.getFrom());
			}

			// Micro-operation: Put captured piece on "to" square if capture
			if (move.isCapture())
				m_board.putPiece(captured, move.captureDestinationSquare<Color>());

			// Operation: Put rook back where it was if castle
			else if (move.isKingsideCastle())
				m_board.movePiece(kingsideCastleRookToSquare<Color>(), kingsideCastleRookFromSquare<Color>());
			else if (move.isQueensideCastle())
				m_board.movePiece(queensideCastleRookToSquare<Color>(), queensideCastleRookFromSquare<Color>());
		}

		/**
		 * Makes and unmakes a move. The callback is called in between. This is a super efficient variation recommended for engine use.
		 */
		template <Color Color, typename Callback>
		inline constexpr void test(const Move move, Callback&& callback) noexcept {
			// TODO:                      UNIMPLEMENTED
			CHESS_ASSERT(false);
		}
	};
}