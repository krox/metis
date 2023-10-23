#include "metis/uci.h"
#include "metis/board.h"
#include "metis/engine.h"
#include <iostream>

namespace metis {

namespace {
bool match(std::string_view &s, std::string_view prefix)
{
	if (s.starts_with(prefix) &&
	    (s.size() == prefix.size() || s[prefix.size()] == ' '))
	{
		s.remove_prefix(prefix.size());
		return true;
	}
	return false;
}
} // namespace

void UCIHandler::stop()
{
	if (thread_.joinable())
	{
		thread_.request_stop();
		thread_.join();
	}
}

void UCIHandler::go(std::string_view cmd)
{
	// fmt::print("going with '{}'\n", cmd);
	(void)cmd;
	// abort previous search (without printing bestmove)
	if (thread_.joinable())
	{
		thread_.request_stop();
		thread_.join();
	}

	// lazy init of the engine
	// (uci protocol suggests to do this not eagerly)
	if (engine_ == nullptr)
		engine_ = std::make_unique<MateInOneEngine>();

	auto thread_main = [this](std::stop_token stoken) {
		engine_->think(
		    Board(board_),
		    [this](AnalysisResult const &r) {
			    best_move_ = r.best_move;
			    // could print a status msg here
		    },
		    stoken);

		respond("bestmove {}", best_move_);
	};

	thread_ = std::jthread(thread_main);
}

void UCIHandler::run()
{
	// create log file for debugging using fmt
	log("starting UCI handler\n");

	std::string line;
	while (std::getline(std::cin, line))
	{
		log("received: '{}'\n", line);
		std::string_view cmd = util::trim_white(line);
		if (cmd.empty())
			continue;

		if (match(cmd, "uci"))
		{
			respond("id name Metis");
			respond("uciok");
		}
		else if (match(cmd, "isready"))
		{
			respond("readyok");
		}
		else if (match(cmd, "ucinewgame"))
		{
			// TODO: clear transposition table or something?
		}
		else if (match(cmd, "position"))
		{
			board_ = Board::from_uci(cmd);
		}

		else if (match(cmd, "go"))
		{
			go(cmd);
		}

		else if (match(cmd, "stop"))
		{
			stop();
		}

		else if (match(cmd, "setoption"))
		{
			/* ignored */
		}
		else if (match(cmd, "quit"))
		{
			if (thread_.joinable())
			{
				thread_.request_stop();
				thread_.join();
			}
			return;
		}
		else
			log("ignoring unknown command: {}\n", cmd);
	}
}

} // namespace metis