/**
 * A fast chess library for C++
 */
#pragma once
#include "game.hpp"
#include "movegen.hpp"
#include <utility>

/**
 * @file Provides helper functions for the user to make the API super easy to use!
 */
namespace chess {
	/**
	 * Dispatches the current player turn to a templated function. Essentially,
	 * a call such as `dispatchRuntimeColor(game, callback, 5)` with `game.turn()`
	 * being `Color::White` will resolve to `callback<Color::White>(game, 5)`.
	 */
	template <typename ProbablyConstGame, typename Callback, typename... Params>
		requires std::same_as<std::remove_cvref_t<ProbablyConstGame>, Game>
	inline constexpr auto dispatchRuntimeColor(ProbablyConstGame&& game, Callback&& callback, Params&&... params)
		noexcept(noexcept(callback.template operator()<Color::White>(std::forward<ProbablyConstGame>(game), std::forward<Params>(params)...)) &&
				 noexcept(callback.template operator()<Color::Black>(std::forward<ProbablyConstGame>(game), std::forward<Params>(params)...)))
	{
		if (game.turn() == Color::White)
			return callback.template operator()<Color::White>(std::forward<ProbablyConstGame>(game), std::forward<Params>(params)...);
		
		return callback.template operator()<Color::Black>(std::forward<ProbablyConstGame>(game), std::forward<Params>(params)...);
	}

	/**
	 * Helper macro that wraps code that needs a compile-time `Color` parameter from the current turn.
	 * The compile-time `Color` parameter will always be named as such, for simplicity.
	 */
#define CHESS_DISPATCH_RUNTIME_COLOR_PARAMETERLESS(game, ...) \
	::chess::dispatchRuntimeColor(game, [&]<::chess::Color Color>(auto&&) __VA_ARGS__)

	/**
	 * \param str The square string, for instance, e7.
	 * \returns The square, Square::None if invalid.
	 * 
	 * Converts a square string into a square number. This function is case-sensitive.
	 */
	inline constexpr int convertToSquare(const std::string_view str) noexcept {
		if (str.length() != 2)
			return Square::None;
		
		const char letter = str[0], number = str[1];
		if (letter < 'a' || letter > 'h' || number < '1' || number > '8')
			return Square::None;
		
		return (int)(number - '1') * 8 + (int)(letter - 'a');
	}

	/**
	 * Represents raw move data. If you want to play a move from this data structure,
	 * you must:
	 *   1) check for pseudolegality and convert it to a suitable Move object
	 *   2) now, with the actual Move object, make the move and check for legality
	 *   3) if illegal, then unmake the move
	 * 
	 * If you want to play a UCI-compatible move string, then you must:
	 *   1) convert the string into a RawMove object
	 *   2) perform the three steps outlined above
	 * 
	 * TODO: Implement RawMove
	 */
	struct RawMove {
		int start = Move::null().getFrom();
		int end = Move::null().getTo();
		PieceType promotion = PieceType::NoPromotion;
	};

	/**
	 * \tparam Color The current turn.
	 * \param game The game context of the move.
	 * \param str The stringified move, for instance, e7e8q.
	 * \returns The resulting Move object, null move if invalid.
	 * 
	 * Converts a UCI move string into a Move object. This function is case-sensitive.
	 * 
	 * \note This function only checks for pseudolegality! In fact, pseudolegality is an
	 *       important precondition for functions like `isLegalPosition`. The generated
	 *       Move object will be pseudolegal only, so legality must be checked elsewhere.
	 */
	template <Color Color>
	inline constexpr Move convertToMove(const Game& game, const std::string_view str) noexcept {
		CHESS_ASSERT_COLOR;

		const size_t length = str.length();
		if (length != 4 && length != 5)
			return Move::null();
		
		const int startSquare = convertToSquare(str.substr(0, 2));
		const int endSquare = convertToSquare(str.substr(2, 2));

		if (startSquare == Square::None || endSquare == Square::None)
			return Move::null();
		
		const Piece pieceFrom = game.board().pieceAt(startSquare);
		const Piece pieceTo = game.board().pieceAt(endSquare);

		// The following are obviously illegal:
		//   1) no piece to move
		//   2) piece of wrong color
		//   3) capture friendly piece
		if (pieceFrom == Piece::None ||
			getPieceColor(pieceFrom) != Color ||
			(pieceTo != Piece::None && getPieceColor(pieceTo) == Color)
		   )
			return Move::null();
		
		const Bitboard startSpot = 1ull << startSquare;
		const Bitboard endSpot = 1ull << endSquare;

		// Destination can either be empty or enemy
		const bool destEmpty = pieceTo == Piece::None;
		
		// Switch across piece types
		switch (getPieceType(pieceFrom)) {
			case PieceType::Pawn: {
				if (startSpot & pawnLastRank<Color>()) {
					// Promotion flag
					MoveFlags promo = MoveFlags::Zero;
					MoveFlags promoCapture = MoveFlags::Zero;

					if (length == 5) {
						switch (str[4]) {
							case 'q': promo = MoveFlags::QueenPromotion; promoCapture = MoveFlags::QueenPromotionCapture; break;
							case 'r': promo = MoveFlags::RookPromotion; promoCapture = MoveFlags::RookPromotionCapture; break; 
							case 'b': promo = MoveFlags::BishopPromotion; promoCapture = MoveFlags::BishopPromotionCapture; break;
							case 'n': promo = MoveFlags::KnightPromotion; promoCapture = MoveFlags::KnightPromotionCapture; break;
						}
					}

					if (forward<Color>(startSpot) == endSpot && destEmpty) {
						return Move{startSquare, endSquare, promo};
					} else if (movegen::leftPawnAttack<Color>(startSpot) == endSpot && !destEmpty) {
						return Move{startSquare, endSquare, promoCapture};
					} else if (movegen::rightPawnAttack<Color>(startSpot) == endSpot && !destEmpty) {
						return Move{startSquare, endSquare, promoCapture};
					}
				} else {
					if (forward<Color>(startSpot) == endSpot && destEmpty) {
						return Move{startSquare, endSquare, MoveFlags::QuietMove};
					} else if (doubleForward<Color>(startSpot & pawnStartingRank<Color>()) == endSpot &&
					           game.board().pieceAt(forwardSquare<Color>(startSquare)) == Piece::None &&
					           destEmpty) {
						return Move{startSquare, endSquare, MoveFlags::DoublePawnPush};
					} else if (movegen::leftPawnAttack<Color>(startSpot) == endSpot && !destEmpty) {
						return Move{startSquare, endSquare, MoveFlags::Capture};
					} else if (movegen::rightPawnAttack<Color>(startSpot) == endSpot && !destEmpty) {
						return Move{startSquare, endSquare, MoveFlags::Capture};
					} else {
						const int epSquare = game.enPassantSquare();

						if (epSquare == endSquare && (startSpot & pawnEnPassantRank<Color>()) != 0) {
							const Bitboard epSpot = 1ull << epSquare;

							if ((movegen::leftPawnAttack<Color>(startSpot) == epSpot ||
							    movegen::rightPawnAttack<Color>(startSpot) == epSpot) && destEmpty) {
								return Move{startSquare, endSquare, MoveFlags::EnPassantCapture};
							}
						}
					}
				}
			
				return Move::null();
			}

			case PieceType::Knight: {
				if (lookup::knightAttack(startSquare) & endSpot)
					return Move{startSquare, endSquare, destEmpty ? MoveFlags::QuietMove : MoveFlags::Capture};
				
				return Move::null();
			}

			case PieceType::King: {
				// What squares should be unoccupied during castling
				constexpr Bitboard shouldUnoccupiedKingside =
					squaresBetweenUnordered(kingsideCastleRookFromSquare<Color>(), initialKingSquare<Color>());
				constexpr Bitboard shouldUnoccupiedQueenside =
					squaresBetweenUnordered(queensideCastleRookFromSquare<Color>(), initialKingSquare<Color>());

				if (lookup::kingAttack(startSquare) & endSpot) {
					return Move{startSquare, endSquare, destEmpty ? MoveFlags::QuietMove : MoveFlags::Capture};
				} else if (startSquare == initialKingSquare<Color>() &&
				           endSquare == kingsideCastleKingToSquare<Color>() &&
				           (game.board().occupied() & shouldUnoccupiedKingside) == 0) {
					return Move{startSquare, endSquare, MoveFlags::KingCastle};
				} else if (startSquare == initialKingSquare<Color>() &&
				           endSquare == queensideCastleKingToSquare<Color>() &&
				           (game.board().occupied() & shouldUnoccupiedQueenside) == 0) {
					return Move{startSquare, endSquare, MoveFlags::QueenCastle};
				}
				
				return Move::null();
			}

			case PieceType::Bishop: {
				if (lookup::bishopAttack(startSquare, game.board().occupied()) & endSpot)
					return Move{startSquare, endSquare, destEmpty ? MoveFlags::QuietMove : MoveFlags::Capture};
				
				return Move::null();
			}

			case PieceType::Rook: {
				if (lookup::rookAttack(startSquare, game.board().occupied()) & endSpot)
					return Move{startSquare, endSquare, destEmpty ? MoveFlags::QuietMove : MoveFlags::Capture};
				
				return Move::null();
			}

			case PieceType::Queen: {
				if (lookup::queenAttack(startSquare, game.board().occupied()) & endSpot)
					return Move{startSquare, endSquare, destEmpty ? MoveFlags::QuietMove : MoveFlags::Capture};
				
				return Move::null();
			}
		}

		CHESS_ASSERT(false); // should not happen at all
		return Move::null();
	}
}