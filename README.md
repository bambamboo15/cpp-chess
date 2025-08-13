# Chess

This is a lightning-fast templated header-only bitboard chess API for C++20 that supports move make/unmake, flexible move generation routines, Zobrist hashing, along with many other features. The move generation has a speed of roughly 500,000,000 nodes per second (with bulk-counted performance testing), making this one of the fastest move generators ever.

> This is a project I've been working a little on recently. Many features are incomplete or work-in-progress.

Normally, move generators of this speed don't provide much of an API, often being tightly coupled with the relevant engine. However, this repository provides a usable API.

## Examples
The below code enumerates all legal moves that can be made from the starting position, in two different ways:

```cpp
#include <iostream>
#include <chess/chess.hpp>
using namespace chess;

int main() {
	// Initializes all of the necessary lookup tables and game state.
	Game game;

	// Enumerates all legal moves from White's perspective. Notice the callback
	// as the second argument; there is no need to enumerate all moves into
	// a move list, and then enumerate over them again to print them out.
	// This can all be done in one enumeration.
	movegen::legalMoves<Color::White>(game, [](const Move move) {
		std::cout << move;
	});
}
```

Let's say you want to choose a random move for the current player. You can do that with the flexible `MoveList` API!

```cpp
#include <iostream>
#include <chess/chess.hpp>
using namespace chess;

int main() {
	Game game;

	// A MoveList object can be cleanly dropped in as the callback to move generation.
	MoveList moveList;
	movegen::legalMoves<Color::White>(game, moveList);
	
	std::cout << moveList.random();
}
```

Imagine you want to make a move, analyze the position, then undo that move. You can do that easily:

```cpp
#include <chess/chess.hpp>
using namespace chess;

// Tests if a move gives check.
template <Color Color>
bool givesCheck(Game& game, const Move move) {
	// Play the move
	const UndoInfo undoInfo = game.make<Color>(move);

	// Does this give check?
	const bool inCheck = movegen::isCheck<~Color>(game);

	// Undo the move
	game.unmake<Color>(move, undoInfo);

	return inCheck;
}
```