#!/usr/bin/env python3

import chess
import chess.engine
import time
import argparse
from tqdm import tqdm


def make_line(moves, result):
    r = ""
    for move in moves:
        r += move.uci() + " "
    r += f"| {result}"
    return r


def generate_selfplay_games(engine_path, num_games,
                            max_moves, time_limit, filename):
    limit = chess.engine.Limit(time=time_limit)

    white_wins = 0
    black_wins = 0
    draws = 0
    total_moves = 0

    with chess.engine.SimpleEngine.popen_uci(engine_path) as engine, open(filename, "w") as f, tqdm(total=num_games, desc="Generating games", unit="game") as bar:
        start_time = time.perf_counter()
        for _ in range(num_games):
            board = chess.Board()
            moves = []
            while not board.is_game_over() and board.fullmove_number < max_moves:
                result = engine.play(board, limit)
                moves.append(result.move)
                board.push(result.move)
                total_moves += 1

            res = board.result()
            f.write(make_line(moves, res) + "\n")

            if res == "1-0":
                white_wins += 1
            elif res == "0-1":
                black_wins += 1
            elif res == "1/2-1/2" or res == "*":
                # note: '*' indicates unfinished game
                draws += 1
            else:
                assert False, f"Unknown result: {res}"

            avg_move_time = (
                time.perf_counter() - start_time) / total_moves

            bar.update(1)
            bar.set_postfix({
                "s/move": f"{avg_move_time:.3f}",
                "moves": total_moves,
                "WDL": f"{white_wins},{draws},{black_wins}"
            }, refresh=False)
    total_written = white_wins + black_wins + draws

    print(f"Generation done. {total_written} game(s) written to {filename}.")
    print(
        f"Summary: {white_wins} white wins, {black_wins} black wins, {draws} draws")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Generate self-play games using a UCI engine.")
    parser.add_argument(
        "--engine-path",
        default="stockfish",
        help="Path to UCI engine executable")
    parser.add_argument(
        "--num-games",
        type=int,
        default=500,
        help="Number of games to generate")
    parser.add_argument(
        "--max-moves",
        type=int,
        default=200,
        help="Maximum fullmove number per game")
    parser.add_argument(
        "--time-limit",
        type=float,
        default=0.01,
        help="Engine time limit per move (seconds)")
    parser.add_argument(
        "--filename",
        default="selfplay.txt",
        help="output filename")
    args = parser.parse_args()

    generate_selfplay_games(
        engine_path=args.engine_path,
        num_games=args.num_games,
        max_moves=args.max_moves,
        time_limit=args.time_limit,
        filename=args.filename,
    )
