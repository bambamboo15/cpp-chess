/**
 * A fast chess library for C++
 */
#pragma once
#include "game.hpp"
#include "movelist.hpp"
#include "lookup.hpp"

namespace chess {
	namespace movegen {
		namespace detail {
			struct MoveCounterBase { };

			template <typename Callback>
			concept isMoveCounter = std::is_base_of_v<MoveCounterBase, std::remove_cvref_t<Callback>>;

			struct MoveCounter : MoveCounterBase {
				int count = 0;

				inline constexpr void operator()(auto&&) const noexcept { }
			};
		}

		template <Color Color>
		inline constexpr Bitboard rightPawnAttack(const Bitboard pawns) noexcept {
			return forward<Color>(pawns & ~FileMask::hFile) << 1;
		}

		template <Color Color>
		inline constexpr Bitboard leftPawnAttack(const Bitboard pawns) noexcept {
			return forward<Color>(pawns & ~FileMask::aFile) >> 1;
		}

		// The reverse of a right pawn capture
		template <Color Color>
		inline constexpr Bitboard reverseRightPawnAttack(const Bitboard right) noexcept {
			return forward<~Color>(right & ~FileMask::aFile) >> 1;
		}

		// The reverse of a left pawn capture
		template <Color Color>
		inline constexpr Bitboard reverseLeftPawnAttack(const Bitboard left) noexcept {
			return forward<~Color>(left & ~FileMask::hFile) << 1;
		}

		template <Color Color>
		inline constexpr Bitboard computeCheckmask(const Game& game) noexcept {
			// ================================ EXPLANATION OF CONCEPT ================================
			//
			// The checkmask is a key part of legal move detection. Essentially, when a king is in check,
			// the non-king pieces can either block the check or take the checking piece. When there is
			// no check, the checkmask is all squares, but when the king is in check, the checkmask is
			// just the path to the king, including the checking piece. This means that we can simply
			// AND a non-king move with the checkmask to prune all non-king moves that do not take care
			// of the check.
			//
			// For double check, the checkmask is simply zero, which would cause non-king pieces to be
			// unable to move. This is expected and comes naturally in the below code.

			Bitboard checkmask = 0xFFFFFFFFFFFFFFFFull;

			const Board& board = game.board();
			const Bitboard king = board.kings<Color>();
			const int square = toSquare(king);

			// All different enemy pieces.
			const Bitboard enemyPawns = board.pawns<~Color>();
			const Bitboard enemyKnights = board.knights<~Color>();
			const Bitboard enemyBishops = board.bishops<~Color>();
			const Bitboard enemyRooks = board.rooks<~Color>();
			const Bitboard enemyQueens = board.queens<~Color>();
			const Bitboard enemyKings = board.kings<~Color>();
			
			// Compute checkmask after considering rook checks.
			//
			// You may think that two of the same slider will never double check a king, but this is wrong.
			// Consider promotion as an edge case. However, it is assumed that two bishops cannot perform
			// a double check.
			const Bitboard rookAttack = lookup::rookAttack(square, board.occupied());
			if (const Bitboard checker = rookAttack & (enemyRooks | enemyQueens)) {
				// The reason why double check can happen with two rook sliders:
				//   https://lichess.org/editor/4kn2/4P3/8/8/4Q3/4K3/8/8_w_-_-_0_1?color=white (e7f8q)
				if ((checker & (checker - 1)) != 0) {
					checkmask = 0;
				} else {
					// The checkmask should not contain the king but should contain the checking piece.
					checkmask &= rookAttack & (lookup::rookAttack(toSquare(checker), board.occupied()) | checker);
				}
			}

			// Check for bishop attacks.
			const Bitboard bishopAttack = lookup::bishopAttack(square, board.occupied());
			if (const Bitboard checker = bishopAttack & (enemyBishops | enemyQueens)) {
				// Two bishop attacks at once can never happen
				CHESS_ASSERT((checker & (checker - 1)) == 0);
				
				checkmask &= bishopAttack & (lookup::bishopAttack(toSquare(checker), board.occupied()) | checker);
			}

			// If a knight is checking the king, then the checkmask will be just that knight.
			//
			// We assume valid chess positions only, so a double check must be done with at least one slider.
			// This means that a double check cannot occur with knights or pawns only.
			const Bitboard knightAttack = lookup::knightAttack(square);
			if (const Bitboard checker = knightAttack & enemyKnights) {
				CHESS_ASSERT((checker & (checker - 1)) == 0);

				checkmask &= checker;
			}

			// Same thing as with knights, but this time with pawns.
			const Bitboard pawnAttack = leftPawnAttack<Color>(king) | rightPawnAttack<Color>(king);
			if (const Bitboard checker = pawnAttack & enemyPawns) {
				CHESS_ASSERT((checker & (checker - 1)) == 0);

				checkmask &= checker;
			}
			
			// The king will never check the opposing king, so checking for that is unnecessary.

			return checkmask;
		}

		// This function outputs the squares that the enemy attacks without considering the king.
		// That is, attacks can go through the king.
		template <Color Color>
		inline constexpr Bitboard computeAttackedWithoutKing(const Game& game) noexcept {
			const Board& board = game.board();
			const Bitboard king = board.kings<Color>();
			Bitboard banned = 0;

			// Calculate attack from enemy pawns
			const Bitboard pl = leftPawnAttack<~Color>(board.pawns<~Color>());
			const Bitboard pr = rightPawnAttack<~Color>(board.pawns<~Color>());
			banned |= pl | pr;

			// Calculate attack from enemy king
			banned |= lookup::kingAttack(toSquare(board.kings<~Color>()));

			// Calculate attack from enemy knights
			Bitboard enemyKnights = board.knights<~Color>();
			while (enemyKnights != 0)
				banned |= lookup::knightAttack(popLSB(enemyKnights));

			// Calculate attack from enemy bishops (do not include king, to stop king from going backwards but still in check)
			// (XOR is used instead of &~ as micro-optimization)
			Bitboard enemyBishops = board.bishops<~Color>() | board.queens<~Color>();
			while (enemyBishops != 0)
				banned |= lookup::bishopAttack(popLSB(enemyBishops), board.occupied() ^ king);

			// Calculate attack from enemy rooks
			Bitboard enemyRooks = board.rooks<~Color>() | board.queens<~Color>();
			while (enemyRooks != 0)
				banned |= lookup::rookAttack(popLSB(enemyRooks), board.occupied() ^ king);
			
			// Compute king legal moves
			return banned;
		}

		// This function outputs a mask of the current horizontal or vertical pin paths through relevant pinned pieces.
		template <Color Color>
		inline constexpr Bitboard computeHorizontalVerticalPinmask(const Game& game) noexcept {
			const Board& board = game.board();
			const Bitboard king = board.kings<Color>();
			const int kingSquare = toSquare(king);
			const Bitboard enemyRooks = board.rooks<~Color>() | board.queens<~Color>();

			// All potentially horizontally or vertically pinned pieces
			const Bitboard probe = lookup::rookAttack(kingSquare, board.occupied());
			const Bitboard potentiallyPinned = probe & board.occupancy<Color>();

			// Look through those pinned pieces for enemy rooks, like an X-ray
			// Pieces that check the king directly would have been caught by the first probe, so discard those pieces
			const Bitboard xray = lookup::rookAttack(kingSquare, board.occupied() & ~potentiallyPinned);
			Bitboard pinners = xray & enemyRooks & ~probe;

			// Look through each pinner
			Bitboard pinmask = 0;
			while (pinners != 0) {
				const int pinnerSquare = popLSB(pinners);

				// If the pinner is attacking a potentially pinned piece, then that piece is surely pinned
				const Bitboard pinnedPieceSpot = lookup::rookAttack(pinnerSquare, board.occupied()) & potentiallyPinned;
				
				// Obtain the path of the pin in a clever way
				pinmask |= (lookup::rookAttack(toSquare(pinnedPieceSpot), board.occupied()) | pinnedPieceSpot) & xray;
			}

			return pinmask;
		}

		// This function outputs a mask of the current diagonal pin paths through relevant pinned pieces.
		template <Color Color>
		inline constexpr Bitboard computeDiagonalPinmask(const Game& game) noexcept {
			// This function is essentially just the same logic as the similar computeHVPinmask function,
			// so I will skip commenting this.

			const Board& board = game.board();
			const Bitboard king = board.kings<Color>();
			const int kingSquare = toSquare(king);
			const Bitboard enemyBishops = board.bishops<~Color>() | board.queens<~Color>();

			const Bitboard probe = lookup::bishopAttack(kingSquare, board.occupied());
			const Bitboard potentiallyPinned = probe & board.occupancy<Color>();

			const Bitboard xray = lookup::bishopAttack(kingSquare, board.occupied() & ~potentiallyPinned);
			Bitboard pinners = xray & enemyBishops & ~probe;

			Bitboard pinmask = 0;
			while (pinners != 0) {
				const Bitboard pinnedPieceSpot = lookup::bishopAttack(popLSB(pinners), board.occupied()) & potentiallyPinned;

				pinmask |= (lookup::bishopAttack(toSquare(pinnedPieceSpot), board.occupied()) | pinnedPieceSpot) & xray;
			}

			return pinmask;
		}

		template <Color Color, typename Callback>
		inline constexpr void legalMoves(const Game& game, Callback&& callback) noexcept {
			CHESS_PROFILE;

			// Obtain all pieces
			const Board& board = game.board();
			const Bitboard pawns = board.pawns<Color>();
			const Bitboard knights = board.knights<Color>();
			const Bitboard bishops = board.bishops<Color>();
			const Bitboard rooks = board.rooks<Color>();
			const Bitboard queens = board.queens<Color>();
			const Bitboard king = board.kings<Color>();

			// Compute checkmask
			const Bitboard checkmask = computeCheckmask<Color>(game);

			// Compute pinmasks
			const Bitboard pinHV = computeHorizontalVerticalPinmask<Color>(game);
			const Bitboard pinD = computeDiagonalPinmask<Color>(game);

			// Obtain the squares that are moveable to for non-pawn non-king pieces
			const Bitboard moveable = ~board.occupancy<Color>() & checkmask;

			// Generate legal pawn moves
			{
				// Obtain pawns that are not pinned (in certain directions)
				const Bitboard pawnsUHV = pawns & ~pinHV;
				const Bitboard pawnsUD = pawns & ~pinD;

				// Calculate the four possible normal pawn moves:
				//   quiet pawn moves, double pawn push, left capture, right capture
				Bitboard quiet = pawnsUD & forward<~Color>(~board.occupied());
				Bitboard doublePush = quiet & pawnStartingRank<Color>() & doubleForward<~Color>(~board.occupied() & checkmask);
				Bitboard leftCapture = pawnsUHV & reverseLeftPawnAttack<Color>(board.occupancy<~Color>() & checkmask);
				Bitboard rightCapture = pawnsUHV & reverseRightPawnAttack<Color>(board.occupancy<~Color>() & checkmask);
				quiet &= forward<~Color>(checkmask);

				// Pruning quiet pawn moves:
				//   We already pruned all D-pinned pawns, so we can split pawns into HV-pinned and unpinned.
				//   HV-pinned pawns can only perform a quiet pawn move if the resulting square is still on
				//   the HV-pinmask.
				const Bitboard quietPinned = quiet & pinHV;
				const Bitboard quietUnpinned = quiet & ~pinHV;

				quiet = (quietPinned & forward<~Color>(pinHV)) | quietUnpinned;

				// Pruning double push pawn moves:
				//   Same logic as quiet pawn moves.
				const Bitboard doublePushPinned = doublePush & pinHV;
				const Bitboard doublePushUnpinned = doublePush & ~pinHV;

				doublePush = (doublePushPinned & doubleForward<~Color>(pinHV)) | doublePushUnpinned;

				// Pruning left captures:
				//   We have already pruned all HV-pinned left captures. This means that left captures can be
				//   split into D-pinned and unpinned. Such pinned captures can only happen when the resulting
				//   square is in the D-pinmask.
				const Bitboard leftCapturePinned = leftCapture & pinD;
				const Bitboard leftCaptureUnpinned = leftCapture & ~pinD;

				leftCapture = (leftCapturePinned & reverseLeftPawnAttack<Color>(pinD)) | leftCaptureUnpinned;

				// Pruning right captures:
				//   Same logic as left captures.
				const Bitboard rightCapturePinned = rightCapture & pinD;
				const Bitboard rightCaptureUnpinned = rightCapture & ~pinD;

				rightCapture = (rightCapturePinned & reverseRightPawnAttack<Color>(pinD)) | rightCaptureUnpinned;

				// Promotion or not?
				Bitboard quietPromotion = quiet & pawnLastRank<Color>();
				Bitboard leftCapturePromotion = leftCapture & pawnLastRank<Color>();
				Bitboard rightCapturePromotion = rightCapture & pawnLastRank<Color>();
				quiet &= ~pawnLastRank<Color>();
				leftCapture &= ~pawnLastRank<Color>();
				rightCapture &= ~pawnLastRank<Color>();
				
				// ================== MOVE COUNT ==================
				if constexpr (detail::isMoveCounter<Callback>) {
					callback.count +=
						popcount(quiet) +
						popcount(doublePush) +
						popcount(leftCapture) +
						popcount(rightCapture) +
						4 * (
							popcount(quietPromotion) +
							popcount(leftCapturePromotion) +
							popcount(rightCapturePromotion)
						);
					
					// If en-passant is available...
					if (const int epSquare = game.enPassantSquare(); epSquare != Square::None) {
						const Bitboard epSpot = 1ull << epSquare;
						const Bitboard epTarget = forward<~Color>(epSpot);

						Bitboard leftEP = pawnsUHV & ~FileMask::aFile & ((epTarget & checkmask) << 1);
						Bitboard rightEP = pawnsUHV & ~FileMask::hFile & ((epTarget & checkmask) >> 1);

						if ((leftEP | rightEP) != 0 && ((leftEP != 0 && rightEP != 0) ||
							(lookup::rookAttack(toSquare(king), board.occupied() ^ (leftEP | rightEP | epSpot | epTarget)) &
								(board.rooks<~Color>() | board.queens<~Color>())) == 0)) {
							leftEP = (leftEP & pinD & reverseLeftPawnAttack<Color>(pinD)) | (leftEP & ~pinD);
							rightEP = (rightEP & pinD & reverseRightPawnAttack<Color>(pinD)) | (rightEP & ~pinD);

							callback.count += (leftEP ? 1 : 0) + (rightEP ? 1 : 0);
						}
					}
				} else {
					// If en-passant is available...
					if (const int epSquare = game.enPassantSquare(); epSquare != Square::None) {
						const Bitboard epSpot = 1ull << epSquare;
						const Bitboard epTarget = forward<~Color>(epSpot);

						Bitboard leftEP = pawnsUHV & ~FileMask::aFile & ((epTarget & checkmask) << 1);
						Bitboard rightEP = pawnsUHV & ~FileMask::hFile & ((epTarget & checkmask) >> 1);

						// Now we start pruning the en-passant moves. We will prune them as normal, but
						// remeber that there is a special case where an en-passant capture can be made
						// by an unpinned pawn, but yet expose the king afterwards. This case will be
						// dealt with separately.
						//
						// This case can really only happen with a horizontal pin. Additionally, when we
						// detect such a case, we can determine that for sure, en-passant is not possible.
						//
						// Our strategy is to remove both pawns involved in en-passant temporarily, and
						// check if any rooks are attacking the king. If there are, then no more!
						//
						// When removing both pawns, we place a pawn on the en-passant capture spot
						// because we do not check for the pin position.
						if ((leftEP | rightEP) != 0 && ((leftEP != 0 && rightEP != 0) ||
							(lookup::rookAttack(toSquare(king), board.occupied() ^ (leftEP | rightEP | epSpot | epTarget)) &
								(board.rooks<~Color>() | board.queens<~Color>())) == 0)) {
							// Prune away pinned en-passant captures.
							leftEP = (leftEP & pinD & reverseLeftPawnAttack<Color>(pinD)) | (leftEP & ~pinD);
							rightEP = (rightEP & pinD & reverseRightPawnAttack<Color>(pinD)) | (rightEP & ~pinD);

							if (leftEP) callback(Move{toSquare(leftEP), epSquare, MoveFlags::EnPassantCapture});
							if (rightEP) callback(Move{toSquare(rightEP), epSquare, MoveFlags::EnPassantCapture});
						}
					}

					// Loop through everything
					while (quiet != 0) {
						const int from = popLSB(quiet);
						const int to = forwardSquare<Color>(from);

						callback(Move{from, to, MoveFlags::QuietMove});
					}

					while (doublePush != 0) {
						const int from = popLSB(doublePush);
						const int to = doubleForwardSquare<Color>(from);

						callback(Move{from, to, MoveFlags::DoublePawnPush});
					}

					while (leftCapture != 0) {
						const int from = popLSB(leftCapture);
						const int to = forwardSquare<Color>(from) - 1;

						callback(Move{from, to, MoveFlags::Capture});
					}

					while (rightCapture != 0) {
						const int from = popLSB(rightCapture);
						const int to = forwardSquare<Color>(from) + 1;

						callback(Move{from, to, MoveFlags::Capture});
					}

					while (quietPromotion != 0) {
						const int from = popLSB(quietPromotion);
						const int to = forwardSquare<Color>(from);

						callback(Move{from, to, MoveFlags::QueenPromotion});
						callback(Move{from, to, MoveFlags::RookPromotion});
						callback(Move{from, to, MoveFlags::KnightPromotion});
						callback(Move{from, to, MoveFlags::BishopPromotion});
					}

					while (leftCapturePromotion != 0) {
						const int from = popLSB(leftCapturePromotion);
						const int to = forwardSquare<Color>(from) - 1;

						callback(Move{from, to, MoveFlags::QueenPromotionCapture});
						callback(Move{from, to, MoveFlags::RookPromotionCapture});
						callback(Move{from, to, MoveFlags::KnightPromotionCapture});
						callback(Move{from, to, MoveFlags::BishopPromotionCapture});
					}

					while (rightCapturePromotion != 0) {
						const int from = popLSB(rightCapturePromotion);
						const int to = forwardSquare<Color>(from) + 1;

						callback(Move{from, to, MoveFlags::QueenPromotionCapture});
						callback(Move{from, to, MoveFlags::RookPromotionCapture});
						callback(Move{from, to, MoveFlags::KnightPromotionCapture});
						callback(Move{from, to, MoveFlags::BishopPromotionCapture});
					}
				}
			}

			// Generate legal knight moves
			{
				// Notice that pinned knights cannot move, so we can prune those away
				Bitboard unpinnedKnights = knights & ~(pinHV | pinD);
				while (unpinnedKnights != 0) {
					const int from = popLSB(unpinnedKnights);

					Bitboard legal = lookup::knightAttack(from) & moveable;
					
					// ================== MOVE COUNT ==================
					if constexpr (detail::isMoveCounter<Callback>) {
						callback.count += popcount(legal);
					} else {
						while (legal != 0) {
							const int to = popLSB(legal);

							// Determine if the move is a capture or not, and compute the relevant move flag, completely branchless
							const MoveFlags moveFlag = static_cast<MoveFlags>(((board.occupancy<~Color>() >> to) & 1ull) << 2);

							callback(Move{from, to, moveFlag});
						}
					}
				}
			}

			// Generate legal bishop moves -- I have folded queens into this for extra optimization!
			// This makes queen legal move generation add essentially zero overhead!
			{
				// We can prune away the bishops that are pinned horizontally or vertically, as those can't really move
				const Bitboard bishopsQueens = (bishops | queens) & ~pinHV;
				Bitboard unpinnedBishops = bishopsQueens & ~pinD;
				Bitboard pinnedBishops = bishopsQueens & pinD;

				// Loop through pinned and unpinned bishops separately
				while (unpinnedBishops != 0) {
					const int from = popLSB(unpinnedBishops);

					Bitboard legal = lookup::bishopAttack(from, board.occupied()) & moveable;
					
					// ================== MOVE COUNT ==================
					if constexpr (detail::isMoveCounter<Callback>) {
						callback.count += popcount(legal);
					} else {
						while (legal != 0) {
							const int to = popLSB(legal);

							// Determine if the move is a capture or not, and compute the relevant move flag, completely branchless
							const MoveFlags moveFlag = static_cast<MoveFlags>(((board.occupancy<~Color>() >> to) & 1ull) << 2);

							callback(Move{from, to, moveFlag});
						}
					}
				}

				while (pinnedBishops != 0) {
					const int from = popLSB(pinnedBishops);

					// Pinned bishops along diagonal can only move along the diagonal pinmask
					Bitboard legal = lookup::bishopAttack(from, board.occupied()) & moveable & pinD;
					
					// ================== MOVE COUNT ==================
					if constexpr (detail::isMoveCounter<Callback>) {
						callback.count += popcount(legal);
					} else {
						while (legal != 0) {
							const int to = popLSB(legal);

							// Determine if the move is a capture or not, and compute the relevant move flag, completely branchless
							const MoveFlags moveFlag = static_cast<MoveFlags>(((board.occupancy<~Color>() >> to) & 1ull) << 2);

							callback(Move{from, to, moveFlag});
						}
					}
				}
			}

			// Generate legal rook moves
			{
				// We can prune away the rooks that are pinned diagonally since those cannot move
				const Bitboard rooksQueens = (rooks | queens) & ~pinD;
				Bitboard unpinnedRooks = rooksQueens & ~pinHV;
				Bitboard pinnedRooks = rooksQueens & pinHV;

				// Loop through pinned and unpinned rooks separately
				while (unpinnedRooks != 0) {
					const int from = popLSB(unpinnedRooks);

					Bitboard legal = lookup::rookAttack(from, board.occupied()) & moveable;
					
					// ================== MOVE COUNT ==================
					if constexpr (detail::isMoveCounter<Callback>) {
						callback.count += popcount(legal);
					} else {
						while (legal != 0) {
							const int to = popLSB(legal);

							// Determine if the move is a capture or not, and compute the relevant move flag, completely branchless
							const MoveFlags moveFlag = static_cast<MoveFlags>(((board.occupancy<~Color>() >> to) & 1ull) << 2);

							callback(Move{from, to, moveFlag});
						}
					}
				}

				while (pinnedRooks != 0) {
					const int from = popLSB(pinnedRooks);

					// Horizontally or vertically pinned rooks can only move along the relevant pinmask
					Bitboard legal = lookup::rookAttack(from, board.occupied()) & moveable & pinHV;
					
					// ================== MOVE COUNT ==================
					if constexpr (detail::isMoveCounter<Callback>) {
						callback.count += popcount(legal);
					} else {
						while (legal != 0) {
							const int to = popLSB(legal);

							// Determine if the move is a capture or not, and compute the relevant move flag, completely branchless
							const MoveFlags moveFlag = static_cast<MoveFlags>(((board.occupancy<~Color>() >> to) & 1ull) << 2);

							callback(Move{from, to, moveFlag});
						}
					}
				}
			}

			// Generate legal king moves
			{
				const Bitboard banned = computeAttackedWithoutKing<Color>(game);

				const int kingSquare = toSquare(king);
				Bitboard kingMoves = ~banned & lookup::kingAttack(kingSquare) & ~board.occupancy<Color>();

				// What squares should be unoccupied during castling
				constexpr Bitboard shouldUnoccupiedKingside =
					squaresBetweenUnordered(kingsideCastleRookFromSquare<Color>(), initialKingSquare<Color>());
				constexpr Bitboard shouldUnoccupiedQueenside =
					squaresBetweenUnordered(queensideCastleRookFromSquare<Color>(), initialKingSquare<Color>());
				
				// What squares should be not attacked during castling
				constexpr Bitboard shouldNotAttackedKingside =
					squaresBetweenUnordered(kingsideCastleKingToSquare<Color>(), initialKingSquare<Color>()) |
					(1ull << kingsideCastleKingToSquare<Color>()) | (1ull << initialKingSquare<Color>());
				constexpr Bitboard shouldNotAttackedQueenside =
					squaresBetweenUnordered(queensideCastleKingToSquare<Color>(), initialKingSquare<Color>()) |
					(1ull << queensideCastleKingToSquare<Color>()) | (1ull << initialKingSquare<Color>());

				// ================== MOVE COUNT ==================
				if constexpr (detail::isMoveCounter<Callback>) {
					callback.count += popcount(kingMoves);

					// Since (banned & king) will give us if we are in check, we can use that to determine
					// if we are not in check. This would mean that the enemy attacks would not go through
					// our king, which makes (banned) good for use in castling attacked square detection.
					//
					// We can reduce all of the above to one operation: (banned & shouldNotAttacked)
					if ((game.castlingRights() & kingsideCastleFlag<Color>()) != CastlingFlags::None &&
					    (shouldUnoccupiedKingside & board.occupied()) == 0 &&
						(shouldNotAttackedKingside & banned) == 0)
						++callback.count;
					
					if ((game.castlingRights() & queensideCastleFlag<Color>()) != CastlingFlags::None &&
					    (shouldUnoccupiedQueenside & board.occupied()) == 0 &&
						(shouldNotAttackedQueenside & banned) == 0)
						++callback.count;
				} else {
					if ((game.castlingRights() & kingsideCastleFlag<Color>()) != CastlingFlags::None &&
					    (shouldUnoccupiedKingside & board.occupied()) == 0 &&
						(shouldNotAttackedKingside & banned) == 0) {
						callback(Move{kingSquare, kingSquare + 2, MoveFlags::KingCastle});
					}

					if ((game.castlingRights() & queensideCastleFlag<Color>()) != CastlingFlags::None &&
					    (shouldUnoccupiedQueenside & board.occupied()) == 0 &&
						(shouldNotAttackedQueenside & banned) == 0) {
						callback(Move{kingSquare, kingSquare - 2, MoveFlags::QueenCastle});
					}

					while (kingMoves != 0) {
						const int to = popLSB(kingMoves);

						// Determine if the move is a capture or not, and compute the relevant move flag, completely branchless
						const MoveFlags moveFlag = static_cast<MoveFlags>(((board.occupancy<~Color>() >> to) & 1ull) << 2);

						callback(Move{kingSquare, to, moveFlag});
					}
				}
			}
		}

		template <Color Color>
		inline constexpr uint64_t legalMoveCount(const Game& game) noexcept {
			detail::MoveCounter moveCounter;

			legalMoves<Color>(game, moveCounter);

			return moveCounter.count;
		}

		template <Color Color>
		inline constexpr bool squareAttacked(const Board& board, const int square) {
			const Bitboard spot = 1ull << square;

			// All different enemy pieces.
			const Bitboard enemyPawns = board.pawns<~Color>();
			const Bitboard enemyKnights = board.knights<~Color>();
			const Bitboard enemyBishops = board.bishops<~Color>();
			const Bitboard enemyRooks = board.rooks<~Color>();
			const Bitboard enemyQueens = board.queens<~Color>();
			const Bitboard enemyKings = board.kings<~Color>();

			const Bitboard attackers =
				((leftPawnAttack<Color>(spot) | rightPawnAttack<Color>(spot)) & enemyPawns) |
				(lookup::kingAttack(square) & enemyKings) |
				(lookup::knightAttack(square) & enemyKnights) |
				(lookup::bishopAttack(square, board.occupied()) & (enemyBishops | enemyQueens)) |
				(lookup::rookAttack(square, board.occupied()) & (enemyRooks | enemyQueens));
			
			return attackers != 0;
		}

		/**
		 * Returns true if the position is legal given the last played move, false otherwise.
		 * 
		 * Use only with pseudolegal move generation.
		 */
		template <Color Color>
		inline constexpr bool isLegalPosition(const Board& board, const Move move) {
			// If the move was a castle, check multiple squares
			[[unlikely]]
			if (move.isCastle()) {
				// Make sure that the castling squares are not attacked
				constexpr Bitboard shouldNotAttackedKingside =
					squaresBetweenUnordered(initialKingSquare<Color>(), kingsideCastleKingToSquare<Color>()) |
					(1ull << kingsideCastleKingToSquare<Color>()) | (1ull << initialKingSquare<Color>());
				constexpr Bitboard shouldNotAttackedQueenside =
					squaresBetweenUnordered(initialKingSquare<Color>(), queensideCastleKingToSquare<Color>()) |
					(1ull << queensideCastleKingToSquare<Color>()) | (1ull << initialKingSquare<Color>());
				Bitboard shouldNotAttacked = move.isKingsideCastle() ? shouldNotAttackedKingside : shouldNotAttackedQueenside;
				
				while (shouldNotAttacked != 0)
					if (squareAttacked<Color>(board, popLSB(shouldNotAttacked)))
						return false;
			}

			return !squareAttacked<Color>(board, toSquare(board.kings<Color>()));
		}

		/**
		 * \tparam Color The player to check for.
		 * \returns True if the given player is in check, false if not.
		 * 
		 * \note This function may not be the most optimal choice for your situation!
		 * 
		 * Checks if the given player is in check.
		 */
		template <Color Color>
		inline constexpr bool isCheck(const Game& game) noexcept {
			const Board& board = game.board();

			return squareAttacked<Color>(board, toSquare(board.kings<Color>()));
		}

		/**
		 * \tparam Color The current player to move. This should be the opposite color of who moved last.
		 * \returns True if game is drawn, and false if not.
		 * 
		 * Checks if the game is currently drawn. Nothing happens if the game is drawn,
		 * so in order to do something when the game is drawn, you must check this yourself
		 * using this function!
		 */
		template <Color Color>
		inline constexpr bool isDrawn(const Game& game) noexcept {
			// TODO: Repetition detection
			return isStalemate<Color>(game) || game.halfMoveCounter() >= 50;
		}

		/**
		 * \tparam Color The player to check for.
		 * \returns True if the given player is stalemated, false if not.
		 * 
		 * This is quite inefficient, but it's alright for minimal use. This is because
		 * this does a direct `legalMoveCount == 0` with an `isCheck` check. There may be
		 * better ways to do this. Consider checking for stalemate manually, and caching
		 * and optimizing results, such as legal move count.
		 */
		template <Color Color>
		inline constexpr bool isStalemate(const Game& game) noexcept {
			return legalMoveCount<Color>() == 0 && !isCheck<Color>(game);
		}

		/**
		 * \tparam Color The player to check for.
		 * \returns True if the given player is checkmated, false if not.
		 * 
		 * This is quite inefficient, but it's alright for minimal use. This is because
		 * this does a direct `legalMoveCount == 0` with an `isCheck` check. There may be
		 * better ways to do this. Consider checking for checkmate manually, and caching
		 * and optimizing results, such as legal move count.
		 */
		template <Color Color>
		inline constexpr bool isCheckmate(const Game& game) noexcept {
			return isCheck<Color>(game) && legalMoveCount<Color>() == 0;
		}
	}
}