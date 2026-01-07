#!/usr/bin/env python3

import matplotlib.pyplot as plt
import chess
import numpy as np


def board_to_bitboards(board):
    """converts a 'chess.Board' to 2x6 bitboards"""
    bb = np.zeros((2, 6), dtype=np.uint64)
    # note: piece-types in 'chess' are 1-6, so we shift by 1
    for piece_type in range(1, 7):
        bb[0, piece_type - 1] = board.pieces(piece_type, chess.WHITE).mask
        bb[1, piece_type - 1] = board.pieces(piece_type, chess.BLACK).mask
    return bb


def load_file(filename):
    """
    load games from file, one game per line, format
    <move> <move> ... | <result>
    returns:
        data_bb : piece-bitboards for each position (shape = (N,2,6))
        data_y : result of each position (shape = (N,))
    """

    RESULT_TO_EVAL = {
        "0-1": 0.0,
        "1-0": 1.0,
        "1/2-1/2": 0.5,
        "*": 0.5,  # unfinished games treated as draw
    }

    data_x = []
    data_y = []
    with open(filename, "r") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            moves, result = line.split("|")
            moves = moves.strip().split()
            result = RESULT_TO_EVAL.get(result.strip(), None)

            board = chess.Board()
            for move in moves:
                board.push_uci(move)
                data_x.append(board_to_bitboards(board))
                data_y.append(result)

    data_x = np.array(data_x, dtype=np.uint64)
    data_y = np.array(data_y, dtype=np.float32)
    return data_x, data_y


def bitboards_to_features(bbs, feat_defs):
    """
    compute feature-vector from bitboards. Black-white symmetry is handled
    automatically by counting black pieces negative

    Parameters:
        bbs:  positions as bitboards (shape=(N,2,6), dtype=uint64)
        feat_defs:   List of (piece_type, bitboard) tuples of size K
    Returns:
        features:   feature-vector per position (shape=(N,K), dtype=float32)
    """
    assert bbs.ndim == 3 and bbs.shape[1:] == (2, 6)
    assert bbs.dtype == np.uint64
    x = np.zeros((bbs.shape[0], len(feat_defs)), dtype=np.float32)

    for i, feat in enumerate(feat_defs):
        piece, mask = feat
        assert 1 <= piece <= 6
        white_bb = np.bitwise_and(bbs[:, 0, piece - 1], mask)
        black_bb = np.bitwise_and(bbs[:, 1, piece - 1], mask)
        x[:, i] = np.bitwise_count(white_bb) - 1.0 * np.bitwise_count(black_bb)
    return x


def weights_to_psts(ws, feat_defs):
    """
    convert (learnt) weights to nice human-readable piece-square-tables
    """
    assert ws.shape == (len(feat_defs),)
    psts = np.zeros((6, 8, 8), dtype=np.float32)
    for w, feat in zip(ws, feat_defs):
        piece, mask = feat
        for square in chess.SQUARES:
            if mask & chess.BB_SQUARES[square]:
                row = chess.square_rank(square)
                col = chess.square_file(square)
                psts[piece - 1, row, col] += w
    return psts


def plot_psts(psts):
    fig, axes = plt.subplots(2, 3, figsize=(12, 8))
    axes = axes.flatten()

    for i, ax in enumerate(axes):
        im = ax.imshow(psts[i], cmap="coolwarm")
        ax.set_title(chess.piece_name(i + 1))
        ax.set_xticks([])
        ax.set_yticks([])
        fig.colorbar(im, ax=ax, fraction=0.046, pad=0.04)

    plt.tight_layout()
    plt.show()


def train_logistic_gd(
    X,
    y,
    lr=0.1,
    momentum=0.9,
    l2=0.0,
    max_iters=2000,
    tol=1e-6,
):
    """
    gradient descent with momentum for logistic regression. Loss function is
    the negative log likelihood with (optional) L2 regularization, i.e.,
       L(w) = -1/N sum_i [ y_i log(p_i) + (1 - y_i) log(1 - p_i) ] + 0.5 * l2 * ||w||^2
    where
       p_i = 1 / (1 + exp(-x_i^T w))
    Parameters:
        X         : features, shape (N, D)
        y         : labels, shape (N,). For proper logistic regression, all
                    values should be 0 or 1, but we dont constraint this here
        lr        : learning rate
        momentum  : momentum factor
        l2        : L2 regularization strength
        max_iters : maximum number of iterations
        tol       : tolerance for convergence. If the change in loss is less than tol,
                    we consider the optimization converged
    Returns:
        w         : learned weights
        losses    : array of loss values after each iteration
        converged : bool
    """
    assert X.ndim == 2
    assert y.ndim == 1
    assert X.shape[0] == y.shape[0]
    N, D = X.shape
    w = np.zeros(D)
    v = np.zeros(D)

    losses = []
    loss_prev = np.inf
    converged = False

    for it in range(max_iters):
        z = X @ w
        p = 1.0 / (1.0 + np.exp(-z))

        eps = 1e-9
        p = np.clip(p, eps, 1 - eps)

        loss = -np.mean(y * np.log(p) + (1 - y) * np.log(1 - p))
        if l2 > 0:
            loss += 0.5 * l2 * np.sum(w * w)

        grad = (X.T @ (p - y)) / N
        if l2 > 0:
            grad += l2 * w

        losses.append(loss)

        if abs(loss_prev - loss) < tol:
            converged = True
            break

        v = momentum * v + grad
        w = w - lr * v

        loss_prev = loss

    return w, np.array(losses), converged


# center 4x4 squares
BB_MIDDLE = 66229406269440
features = [
    (chess.PAWN, chess.BB_ALL),
    (chess.KNIGHT, chess.BB_ALL),
    (chess.BISHOP, chess.BB_ALL),
    (chess.ROOK, chess.BB_ALL),
    (chess.QUEEN, chess.BB_ALL),
    # (chess.PAWN, chess.BB_CENTER),
    # (chess.KNIGHT, chess.BB_CENTER),
    # (chess.BISHOP, chess.BB_CENTER),
    # (chess.ROOK, chess.BB_CENTER),
    # (chess.QUEEN, chess.BB_CENTER),
    (chess.PAWN, BB_MIDDLE),
    (chess.KNIGHT, BB_MIDDLE),
    (chess.BISHOP, BB_MIDDLE),
    (chess.ROOK, BB_MIDDLE),
    (chess.QUEEN, BB_MIDDLE),
]


filename = "random_1.txt"

data_bb, data_y = load_file(filename)
print(f"Loaded {data_bb.shape[0]} positions from {filename}")

data_x = bitboards_to_features(data_bb, features)

ws, losses, converged = train_logistic_gd(data_x, data_y)
print(f"converged: {converged} after {len(losses)} iterations")
for w, feat in zip(ws, features):
    piece, mask = feat
    print(f"piece={chess.piece_name(piece)}, mask={mask:016x}, weight={w:.4f}")

psts = weights_to_psts(ws, features)
print(f"psts.shape = {psts.shape}")
plot_psts(psts)
