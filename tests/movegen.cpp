#include "catch2/catch_test_macros.hpp"

#include "metis/board.h"

TEST_CASE("movegen", "[perft]")
{
	// for reference, see https://www.chessprogramming.org/Perft_Results
	SECTION("startpos")
	{
		auto s = metis::GameState();
		CHECK(s.perft(1) == 20);
		CHECK(s.perft(2) == 400);
		CHECK(s.perft(3) == 8902);
		CHECK(s.perft(4) == 197281);
		CHECK(s.perft(5) == 4865609);
	}
	SECTION("kiwipete")
	{
		auto s = metis::GameState(
		    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
		CHECK(s.perft(1) == 48);
		CHECK(s.perft(2) == 2039);
		CHECK(s.perft(3) == 97862);
		CHECK(s.perft(4) == 4085603);
	}
	SECTION("position3")
	{
		auto s = metis::GameState("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -");
		CHECK(s.perft(1) == 14);
		CHECK(s.perft(2) == 191);
		CHECK(s.perft(3) == 2812);
		CHECK(s.perft(4) == 43238);
		CHECK(s.perft(5) == 674624);
	}
	SECTION("position4")
	{
		auto s = metis::GameState(
		    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
		CHECK(s.perft(1) == 6);
		CHECK(s.perft(2) == 264);
		CHECK(s.perft(3) == 9467);
		CHECK(s.perft(4) == 422333);
	}
	SECTION("position4_flipped")
	{
		auto s = metis::GameState(
		    "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1");
		CHECK(s.perft(1) == 6);
		CHECK(s.perft(2) == 264);
		CHECK(s.perft(3) == 9467);
		CHECK(s.perft(4) == 422333);
	}
	SECTION("position5")
	{
		auto s = metis::GameState(
		    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
		CHECK(s.perft(1) == 44);
		CHECK(s.perft(2) == 1486);
		CHECK(s.perft(3) == 62379);
		CHECK(s.perft(4) == 2103487);
	}
	SECTION("position6")
	{
		auto s = metis::GameState("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/"
		                          "P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
		CHECK(s.perft(1) == 46);
		CHECK(s.perft(2) == 2079);
		CHECK(s.perft(3) == 89890);
		CHECK(s.perft(4) == 3894594);
	}
}

TEST_CASE("position key", "[repetition]")
{
	SECTION("same board state after reversible moves")
	{
		auto board = metis::Board::startpos();
		auto start_key = board.zobrist();

		board.make_move(metis::Move("g1f3"));
		board.make_move(metis::Move("b8c6"));
		board.make_move(metis::Move("f3g1"));
		board.make_move(metis::Move("c6b8"));

		CHECK(board.zobrist() == start_key);
	}

	SECTION("castling rights are part of key")
	{
		auto a = metis::Board::from_fen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
		auto b = metis::Board::from_fen("r3k2r/8/8/8/8/8/8/R3K2R w - - 0 1");

		CHECK(!(a.zobrist() == b.zobrist()));
	}

	SECTION("en passant square is part of key")
	{
		auto a = metis::Board::from_fen(
		    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
		auto b = metis::Board::from_fen(
		    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq e3 0 1");

		CHECK(!(a.zobrist() == b.zobrist()));
	}

	SECTION("polyglot vectors")
	{
		auto s0 = metis::Board::from_fen(
		    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
		CHECK(s0.zobrist() == 0x463b96181691fc9cULL);

		auto s1 = metis::Board::from_fen(
		    "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
		CHECK(s1.zobrist() == 0x823c9b50fd114196ULL);

		auto s2 = metis::Board::from_fen(
		    "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2");
		CHECK(s2.zobrist() == 0x0756b94461c50fb0ULL);
	}

	SECTION("startpos moves parser consumes move history")
	{
		auto parsed = metis::GameState::from_uci(
		    "startpos moves g1f3 b8c6 f3g1 c6b8");

		auto manual = metis::Board::startpos();
		manual.make_move(metis::Move("g1f3"));
		manual.make_move(metis::Move("b8c6"));
		manual.make_move(metis::Move("f3g1"));
		manual.make_move(metis::Move("c6b8"));

		CHECK(parsed.board.zobrist() == manual.zobrist());
		CHECK(parsed.history.size() == 5);
		CHECK(parsed.history.back() == parsed.board.zobrist());
		CHECK(parsed.board.zobrist() == metis::Board::startpos().zobrist());
	}

	SECTION("ep square only set when capturable")
	{
		auto a = metis::Board::startpos();
		a.make_move(metis::Move("e2e4"));
		CHECK(a.ep_square == metis::Bitboard::none);

		auto b = metis::Board::from_fen("4k3/8/8/8/3p4/8/4P3/4K3 w - - 0 1");
		b.make_move(metis::Move("e2e4"));
		CHECK(b.ep_square == metis::square((4) + 8 * 2)); // e3
	}
}