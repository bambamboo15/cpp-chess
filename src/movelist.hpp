/**
 * A fast chess library for C++
 */
#pragma once
#include "move.hpp"
#include <random>
#include <algorithm>

namespace chess {
	template <size_t MaxMoves>
	class StaticMoveList {
		Move m_moves[MaxMoves];
		size_t m_count;

	public:
		constexpr StaticMoveList() : m_moves{}, m_count{0} { }

		inline constexpr size_t size() const noexcept {
			return m_count;
		}

		CHESS_ALWAYS_INLINE inline constexpr void add(Move move) noexcept {
			m_moves[m_count++] = move;
		}

		inline Move random() const noexcept {
			if (m_count == 0)
				return { };

			// Despite using thread_local here, MoveList is not thread-safe!
			static thread_local std::mt19937 rng(std::random_device{}());
			std::uniform_int_distribution<size_t> dist(0, m_count - 1);
			return m_moves[dist(rng)];
		}

		template <typename Comparator>
		inline constexpr void sort(Comparator&& comparator) noexcept {
			// TODO: Make more efficient
			std::sort(begin(), end(), comparator);
		}

		// Support as callable
		inline constexpr void operator()(Move move) noexcept {
			add(move);
		}

		// Function to clear the move list
		inline constexpr void clear() noexcept {
			m_count = 0;
		}

		// Iterator support
		inline constexpr Move* begin() noexcept {
			return m_moves;
		}

		inline constexpr Move* end() noexcept {
			return m_moves + m_count;
		}

		inline constexpr const Move* begin() const noexcept {
			return m_moves;
		}

		inline constexpr const Move* end() const noexcept {
			return m_moves + m_count;
		}

		inline constexpr const Move* cbegin() const noexcept {
			return m_moves;
		}

		inline constexpr const Move* cend() const noexcept {
			return m_moves + m_count;
		}

		// Indexing support
		inline constexpr Move& operator[](size_t index) noexcept {
			return m_moves[index];
		}

		inline constexpr const Move& operator[](size_t index) const noexcept {
			return m_moves[index];
		}
	};

	using MoveList = StaticMoveList<218>;
}