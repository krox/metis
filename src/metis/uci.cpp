#include "metis/uci.h"

#include "fmt/os.h"
#include "fmt/ostream.h"
#include "metis/board.h"
#include "metis/engine.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <mutex>
#include <string_view>
#include <thread>

namespace metis {

struct UCI
{
	std::jthread thread_;
	std::unique_ptr<Engine> engine_;
	Board board_ = Board::startpos();
	Move best_move_ = Move::null();
	std::mutex write_mutex_; // to serialize writes to stdout

	// stop current search if any
	void stop();

	// start a new search for current position
	void go();

	// print to stdout (+ newline + flush)
	template <typename... T>
	void respond(fmt::format_string<T...> msg, T &&...args)
	{
		auto lock = std::scoped_lock(write_mutex_);
		std::string response = fmt::format(msg, std::forward<T>(args)...);
		fmt::print("{}\n", response);
		fflush(stdout);
		// future: also log commands here?
	};

	void run();
};

void UCI::go()
{
	stop();

	// lazy init of the engine
	// (uci protocol suggests to do this not eagerly)
	if (engine_ == nullptr)
		engine_ = std::make_unique<CaptureEngine>();

	thread_ = std::jthread([this](std::stop_token stoken) {
		engine_->think(
		    board_,
		    [this](AnalysisResult const &r) {
			    best_move_ = r.best_move;
			    respond("info pv {}",
			            best_move_); // should send plenty more info here
		    },
		    stoken);
		respond("bestmove {}", best_move_);
	});
}

void UCI::stop()
{
	if (thread_.joinable())
	{
		thread_.request_stop();
		thread_.join();
	}
}

void UCI::run()
{
	std::string line;
	while (std::getline(std::cin, line))
	{
		auto lexer = util::Parser(line);
		if (lexer.end())
			continue;

		if (lexer.ident("uci"))
		{
			lexer.expect_end();
			respond("id name Metis");
			respond("uciok");
		}
		else if (lexer.ident("isready"))
		{
			// implicit synchronization point. nothing to do for us really.
			lexer.expect_end();
			respond("readyok");
		}
		else if (lexer.ident("ucinewgame"))
		{
			lexer.expect_end();
			// this indicates the next position will be "from a new game". I
			// think this can be ignored in practice as new positions are set
			// using the "position" command anyways. Should probably reset
			// internal states here.
		}
		else if (lexer.ident("position"))
		{
			board_ = Board::from_fen(lexer);
			lexer.expect_end();
		}

		else if (lexer.ident("go"))
		{
			// ignore any parameters for new...
			go();
		}

		else if (lexer.ident("stop"))
		{
			lexer.expect_end();
			stop();
		}
		else if (lexer.ident("setoption"))
		{
			// ignore any options for now
		}
		else if (lexer.ident("quit"))
		{
			stop();
			return;
		}
		else
			respond("info string ignoring unknown command: {}", line);
	}
}

void run_uci()
{
	UCI uci;
	uci.run();
}

} // namespace metis