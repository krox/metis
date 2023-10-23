#pragma once

#include "fmt/os.h"
#include "fmt/ostream.h"
#include "metis/board.h"
#include "metis/engine.h"
#include <cassert>
#include <memory>
#include <string_view>
#include <thread>

namespace metis {

class UCIHandler
{
	std::unique_ptr<Engine> engine_;
	Board board_ = Board::startpos();
	std::jthread thread_;
	Move best_move_;

	void stop();
	void go(std::string_view cmd);

	std::unique_ptr<fmt::ostream> log_file_;

	// print to stdout (+ newline + flush + log if enabled)
	template <typename... T>
	void respond(fmt::format_string<T...> msg, T &&...args)
	{
		std::string response = fmt::format(msg, std::forward<T>(args)...);

		log("responding: '{}'\n", response);
		fmt::print("{}\n", response);
		fflush(stdout);
	};

	// print to log file (if enabled)
	template <typename... T> void log(fmt::format_string<T...> msg, T &&...args)
	{
		if (log_file_)
		{
			log_file_->print(msg, std::forward<T>(args)...);
			log_file_->flush();
		}
	}

  public:
	UCIHandler(std::unique_ptr<Engine> engine) : engine_(std::move(engine))
	{
		assert(engine_ != nullptr);
	}

	// set_log_file("") to disable logging
	void set_log_file(std::string_view filename)
	{
		if (filename.empty())
			log_file_ = nullptr;
		else
			log_file_ = std::make_unique<fmt::ostream>(
			    fmt::output_file(std::string(filename)));
	}

	// Run the UCI loop, listening and responding to commands on stdin/stdout.
	// Stops upon receiving the "quit" command.
	void run();
};

} // namespace metis