
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <deque>
#include <future>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/movegen.h"
#include "core/position.h"
#include "core/types.h"
#include "minmax/minmax.h"

using namespace tiny;

// ---------- Config ----------
#define WINDOW_W 1920
#define WINDOW_H 1080

static const std::string START_FEN = "fhwk/3p/P3/KWHF w 1";

// Board and UI layout constants (logical)
struct UIConf {
    // Layout constants (static, not computed dynamically)
    int boardSizePx = 800;  // square board edge
    int marginPx    = 24;
    int squarePx    = boardSizePx / 4;

    // Pocket width = one square (same as square size)
    int pocketWidth = squarePx;

    // Compute total content width = pocket + margin + board
    int totalContentWidth = pocketWidth + marginPx + boardSizePx;

    // Center that content horizontally in window
    float startX = (WINDOW_W - totalContentWidth) / 2.0f;
    float startY = (WINDOW_H - boardSizePx) / 2.0f;

    // Pocket on the left
    SDL_FRect leftUIRect = {
        startX,              // x
        startY,              // y
        (float)pocketWidth,  // w
        (float)boardSizePx   // h (same as board)
    };

    // Board to the right of pocket (with margin)
    SDL_FRect boardRect = {
        startX + pocketWidth + marginPx,  // x
        startY,                           // y
        (float)boardSizePx,               // w
        (float)boardSizePx                // h
    };

    // Optional top bar placeholder
    SDL_FRect topBarRect = {startX,
                            startY - (float)(marginPx * 2),  // some offset above
                            (float)totalContentWidth, (float)marginPx * 1.5f};
};

// ---------- Helpers to map types ----------
static inline bool is_own_piece(const Position& pos, Square s) {
    Piece pc = pos.piece_on(s);
    if (pc == NO_PIECE) return false;
    Color c = (pc >= B_PAWN ? BLACK : WHITE);
    return c == pos.side_to_move();
}

// Texture key: piece id (white/black x types).
enum TexKey : int {
    T_W_P = 0,
    T_W_U,
    T_W_F,
    T_W_W,
    T_W_K,
    T_B_P,
    T_B_U,
    T_B_F,
    T_B_W,
    T_B_K,
    TEX_NB
};

static inline TexKey texkey_for_piece(Piece p) {
    switch (p) {
        case W_PAWN:
            return T_W_P;
        case W_HORSE:
            return T_W_U;
        case W_FERZ:
            return T_W_F;
        case W_WAZIR:
            return T_W_W;
        case W_KING:
            return T_W_K;
        case B_PAWN:
            return T_B_P;
        case B_HORSE:
            return T_B_U;
        case B_FERZ:
            return T_B_F;
        case B_WAZIR:
            return T_B_W;
        case B_KING:
            return T_B_K;
        default:
            return T_W_P;
    }
}

static inline TexKey texkey_for_piece(Color c, PieceType pt) {
    if (c == WHITE) {
        switch (pt) {
            case PAWN:
                return T_W_P;
            case HORSE:
                return T_W_U;
            case FERZ:
                return T_W_F;
            case WAZIR:
                return T_W_W;
            case KING:
                return T_W_K;
            default:
                return T_W_P;
        }
    } else {
        switch (pt) {
            case PAWN:
                return T_B_P;
            case HORSE:
                return T_B_U;
            case FERZ:
                return T_B_F;
            case WAZIR:
                return T_B_W;
            case KING:
                return T_B_K;
            default:
                return T_B_P;
        }
    }
}

// ---------- App/Game state ----------
enum class Phase { SideSelect, Playing, PromotionPick, GameOver };

struct PromotionUI {
    // Visible when a user clicked a (from,to) that has multiple promotions.
    Square                   from = SQ_NONE;
    Square                   to   = SQ_NONE;
    std::vector<Move>        options;  // each is PROMOTION with chosen pt
    std::array<SDL_FRect, 3> rects{};  // up to 3 (HORSE, FERZ, WAZIR)
    bool                     visible = false;
};

struct LastMoveVis {
    std::optional<Square> from{};
    std::optional<Square> to{};
};

struct AsyncAI {
    bool                      thinking = false;
    std::future<SearchResult> fut;
};

typedef struct {
    // SDL
    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;

    // Engine
    std::deque<StateInfo> states;
    Position              pos;
    Color                 humanSide = WHITE;

    // Game flow
    Phase phase             = Phase::SideSelect;
    bool  boardFlipped      = false;  // in addition to side orientation; toggled by 'F'
    bool  gameOverCheckmate = false;  // else stalemate or 3fold
    Color winner            = WHITE;  // when game over (checkmate) or stalemate winner

    // Selection
    std::optional<Square>    selectedSq{};
    std::optional<PieceType> selectedDropPiece{};  // when choosing to drop from pocket
    PromotionUI              promo{};
    LastMoveVis              lastMove{};

    // AI
    AsyncAI ai;
    int     searchDepth = 9;

    // Rendering
    UIConf                           ui{};
    std::array<SDL_Texture*, TEX_NB> textures{};
    bool texturesLoaded = false;  // if false, we fallback to primitive drawing

    // Timing
    Uint64 lastTicks = 0;
} AppState;

// ---------- Texture loading ----------
static SDL_Texture* load_texture(SDL_Renderer* r, const char* path) {
    SDL_Texture* texture  = NULL;
    char*        svg_path = NULL;

    SDL_asprintf(&svg_path, "%s%s", SDL_GetBasePath(),
                 path); /* allocate a string of the full file path */
    texture = IMG_LoadTexture(r, svg_path);
    if (!texture) {
        SDL_Log("Couldn't create static texture: %s", SDL_GetError());
    }

    SDL_free(svg_path); /* done with this, the file is loaded. */

    return texture;
}

static void load_all_textures(AppState* as) {
    const char* paths[TEX_NB] = {
        "assets/w_p.svg", "assets/w_h.svg", "assets/w_f.svg", "assets/w_w.svg", "assets/w_k.svg",
        "assets/b_p.svg", "assets/b_h.svg", "assets/b_f.svg", "assets/b_w.svg", "assets/b_k.svg",
    };
    bool ok = true;
    for (int i = 0; i < TEX_NB; ++i) {
        as->textures[i] = load_texture(as->renderer, paths[i]);
        ok              = ok && (as->textures[i] != nullptr);
    }
    as->texturesLoaded = ok;
}

// ---------- Board / UI math ----------
static bool point_in_rect(float x, float y, const SDL_FRect& r) {
    return (x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h);
}

// Map mouse (x,y) to a board Square, or SQ_NONE if outside board
static Square screen_to_square(const AppState* as, float x, float y) {
    const SDL_FRect& B = as->ui.boardRect;
    if (!point_in_rect(x, y, B)) return SQ_NONE;
    const int q   = as->ui.squarePx;
    int       col = int((x - B.x) / q);
    int       row = int((y - B.y) / q);
    if (col < 0 || col >= 4 || row < 0 || row >= 4) return SQ_NONE;

    // Orientation: base on human side (white bottom or black bottom) and flip toggle
    bool whiteView = (as->humanSide == WHITE);
    if (as->boardFlipped) whiteView = !whiteView;

    int file = whiteView ? col : (3 - col);
    int rank = whiteView ? (3 - row) : row;  // row 0 at top of screen; rank 3 is top for white view

    return make_square(File(file), Rank(rank));
}

static SDL_FRect square_rect(const AppState* as, Square s) {
    const int q         = as->ui.squarePx;
    bool      whiteView = (as->humanSide == WHITE);
    if (as->boardFlipped) whiteView = !whiteView;

    int file = file_of(s);
    int rank = rank_of(s);

    int col = whiteView ? file : (3 - file);
    int row = whiteView ? (3 - rank) : rank;

    SDL_FRect r;
    r.x = as->ui.boardRect.x + col * q;
    r.y = as->ui.boardRect.y + row * q;
    r.w = (float)q;
    r.h = (float)q;
    return r;
}

// ---------- Move utilities ----------
static std::vector<Move> legal_moves(const Position& pos) {
    MoveList<LEGAL>   ml(pos);
    std::vector<Move> out;
    out.reserve(ml.size());
    for (int i = 0; i < ml.size(); ++i) out.push_back(ml[i]);
    return out;
}

static std::vector<Move> filter_moves_from(const Position& pos, Square from) {
    std::vector<Move> ms = legal_moves(pos);
    std::vector<Move> out;
    out.reserve(ms.size());
    for (auto m : ms)
        if (m.type_of() == NORMAL || m.type_of() == PROMOTION) {
            if (m.from_sq() == from) out.push_back(m);
        }
    return out;
}

static std::vector<Move> filter_moves_from_to(const Position& pos, Square from, Square to) {
    std::vector<Move> ms = legal_moves(pos);
    std::vector<Move> out;
    for (auto m : ms)
        if (m.from_sq() == from && m.to_sq() == to) out.push_back(m);
    return out;
}

static std::vector<Move> filter_drop_moves(const Position& pos, PieceType pt) {
    std::vector<Move> ms = legal_moves(pos);
    std::vector<Move> out;
    for (auto m : ms)
        if (m.type_of() == DROP && m.drop_piece() == pt) out.push_back(m);
    return out;
}

static std::optional<Move> find_drop_to(const Position& pos, PieceType pt, Square to) {
    auto drops = filter_drop_moves(pos, pt);
    for (auto m : drops)
        if (m.to_sq() == to) return m;
    return std::nullopt;
}

static bool is_terminal(const Position& pos, bool& isMate, Color& winner, bool& isThreefold) {
    MoveList<LEGAL> root(pos);
    if (pos.is_threefold_game()) {
        isThreefold = true;
        return true;
    }
    if (root.size() == 0) {
        isMate = pos.checkers() != 0;
        if (isMate)
            winner = ~pos.side_to_move();
        else
            winner = pos.side_to_move();  // stalemated player wins per rules
        return true;
    }
    return false;
}

// ---------- Rendering ----------
static void draw_rect(SDL_Renderer* r, const SDL_FRect& rc, Uint8 R, Uint8 G, Uint8 B, Uint8 A) {
    SDL_SetRenderDrawColorFloat(r, R / 255.0f, G / 255.0f, B / 255.0f, A / 255.0f);
    SDL_RenderFillRect(r, &rc);
}

static void draw_outline(SDL_Renderer* r, const SDL_FRect& rc, Uint8 R, Uint8 G, Uint8 B, Uint8 A,
                         float thickness = 3.0f) {
    // Draw 4 thin rects as outline
    SDL_SetRenderDrawColorFloat(r, R / 255.0f, G / 255.0f, B / 255.0f, A / 255.0f);
    SDL_FRect t = rc;
    // top
    t.h = thickness;
    SDL_RenderFillRect(r, &t);
    // bottom
    t.y = rc.y + rc.h - thickness;
    t.h = thickness;
    SDL_RenderFillRect(r, &t);
    // left
    t   = rc;
    t.w = thickness;
    SDL_RenderFillRect(r, &t);
    // right
    t.x = rc.x + rc.w - thickness;
    t.w = thickness;
    SDL_RenderFillRect(r, &t);
}

static void draw_circle(SDL_Renderer* r, const SDL_FPoint& c, float radius, Uint8 R, Uint8 G,
                        Uint8 B, Uint8 A) {
    SDL_SetRenderDrawColorFloat(r, R / 255.0f, G / 255.0f, B / 255.0f, A / 255.0f);
    const int N = 40;
    for (int i = 0; i < N; ++i) {
        float      t0 = (float)i * 6.2831853f / N;
        float      t1 = (float)(i + 1) * 6.2831853f / N;
        SDL_FPoint p0{c.x + radius * cosf(t0), c.y + radius * sinf(t0)};
        SDL_FPoint p1{c.x + radius * cosf(t1), c.y + radius * sinf(t1)};
        SDL_RenderLine(r, p0.x, p0.y, p1.x, p1.y);
    }
}

static void draw_board(AppState* as) {
    SDL_Renderer*    r = as->renderer;
    const SDL_FRect& B = as->ui.boardRect;
    draw_rect(r, B, 28, 28, 28, 255);

    const int q = as->ui.squarePx;
    for (int rank = 0; rank < 4; ++rank) {
        for (int file = 0; file < 4; ++file) {
            Square    s    = make_square(File(file), Rank(rank));
            SDL_FRect rc   = square_rect(as, s);
            bool      dark = ((file + rank) & 1);
            Uint8     c    = dark ? 90 : 170;
            draw_rect(r, rc, c, c, c, 255);
        }
    }
}

static void draw_last_move(AppState* as) {
    SDL_Renderer* r = as->renderer;
    if (as->lastMove.from) {
        draw_rect(r, square_rect(as, *as->lastMove.from), 255, 255, 0, 55);
    }
    if (as->lastMove.to) {
        draw_rect(r, square_rect(as, *as->lastMove.to), 0, 255, 0, 55);
    }
}

static void draw_check(AppState* as) {
    // Highlight side-to-move king if in check
    if (as->pos.checkers() == 0) return;
    Square ksq = as->pos.square<KING>(as->pos.side_to_move());
    draw_rect(as->renderer, square_rect(as, ksq), 220, 30, 30, 60);
}

static void draw_piece_texture(SDL_Renderer* r, SDL_Texture* tex, const SDL_FRect& dst) {
    if (!tex) return;
    SDL_RenderTexture(r, tex, nullptr, &dst);
}

static void draw_piece_fallback(SDL_Renderer* r, Color c, PieceType pt, const SDL_FRect& cell) {
    // Simple circle marker; different radii per type to distinguish.
    float rad = std::min(cell.w, cell.h) * (0.35f + 0.05f * (pt - 1));
    Uint8 R   = (c == WHITE ? 240 : 30);
    Uint8 G   = (c == WHITE ? 240 : 30);
    Uint8 B   = (c == WHITE ? 240 : 30);
    draw_circle(r, SDL_FPoint{cell.x + cell.w / 2, cell.y + cell.h / 2}, rad, R, G, B, 255);
}

static void draw_pieces(AppState* as) {
    SDL_Renderer* r = as->renderer;
    for (int s = SQUARE_ZERO; s < SQUARE_NB; ++s) {
        Square sq = Square(s);
        Piece  pc = as->pos.piece_on(sq);
        if (pc == NO_PIECE) continue;
        SDL_FRect rc = square_rect(as, sq);
        TexKey    tk = texkey_for_piece(pc);
        if (as->texturesLoaded && as->textures[tk])
            draw_piece_texture(r, as->textures[tk], rc);
        else {
            Color     c  = (pc >= B_PAWN ? BLACK : WHITE);
            PieceType pt = PieceType(pc & 0x7);  // types are aligned 1..5 per your enum
            draw_piece_fallback(r, c, pt, rc);
        }
    }
}

static void draw_selection(AppState* as) {
    SDL_Renderer* r = as->renderer;

    // Show selected square
    if (as->selectedSq) {
        draw_outline(r, square_rect(as, *as->selectedSq), 40, 200, 255, 255, 5.0f);

        // Show legal targets from that square
        auto ms = filter_moves_from(as->pos, *as->selectedSq);
        for (auto m : ms) {
            SDL_FRect rc = square_rect(as, m.to_sq());
            // Overlay soft highlight (green for quiet, orange if capture)
            bool isCapture = as->pos.piece_on(m.to_sq()) != NO_PIECE;
            draw_rect(r, rc, isCapture ? 255 : 0, isCapture ? 160 : 200, 0, 60);
            // Small dot at center
            draw_circle(r, SDL_FPoint{rc.x + rc.w / 2, rc.y + rc.h / 2}, rc.w * 0.12f, 0, 0, 0,
                        180);
        }
    }

    // Show selected drop piece (pocket), highlight legal drop squares
    if (as->selectedDropPiece) {
        auto drops = filter_drop_moves(as->pos, *as->selectedDropPiece);
        for (auto m : drops) {
            SDL_FRect rc = square_rect(as, m.to_sq());
            draw_rect(r, rc, 60, 200, 60, 60);
            draw_circle(r, SDL_FPoint{rc.x + rc.w / 2, rc.y + rc.h / 2}, rc.w * 0.12f, 0, 0, 0,
                        180);
        }
    }
}

static void draw_pockets(AppState* as) {
    SDL_Renderer*    r = as->renderer;
    const SDL_FRect& U = as->ui.leftUIRect;
    draw_rect(r, U, 24, 24, 40, 255);

    // Two panels: top for side-to-move? Better: fixed — white pocket on bottom, black on top.
    float     pad  = 12.0f;
    float     cell = std::min((U.w - 2 * pad) / 4.0f, (U.h - 3 * pad) / 4.0f);  // 4 columns
    float     tH   = cell * 2 + pad;  // each pocket two rows
    SDL_FRect blackPanel{U.x + pad, U.y + pad, U.w - 2 * pad, tH};
    SDL_FRect whitePanel{U.x + pad, U.y + U.h - pad - tH, U.w - 2 * pad, tH};
    draw_rect(r, blackPanel, 35, 35, 60, 255);
    draw_rect(r, whitePanel, 35, 35, 60, 255);

    auto drawPocket = [&](Color c, const SDL_FRect& panel) {
        // Render PAWN, HORSE, FERZ, WAZIR as up to 2 copies each (Pocket counters are 0..2)
        std::array<PieceType, 4> order{PAWN, HORSE, FERZ, WAZIR};
        float                    gap = cell * 0.2f;
        for (int i = 0; i < 4; ++i) {
            int cnt = as->pos.pocket(c).count(order[i]);
            for (int k = 0; k < std::min(cnt, 2); ++k) {
                SDL_FRect rc{panel.x + i * (cell + gap), panel.y + (k == 0 ? gap : gap * 2 + cell),
                             cell, cell};
                TexKey    tk = texkey_for_piece(c, order[i]);
                if (as->texturesLoaded && as->textures[tk])
                    draw_piece_texture(r, as->textures[tk], rc);
                else
                    draw_piece_fallback(r, c, order[i], rc);

                // If selected drop matches this slot, outline it
                if (as->selectedDropPiece && *as->selectedDropPiece == order[i] &&
                    c == as->pos.side_to_move())
                    draw_outline(r, rc, 0, 255, 255, 255, 4.0f);
            }
        }
    };

    drawPocket(BLACK, blackPanel);
    drawPocket(WHITE, whitePanel);
}

static void draw_promotion_overlay(AppState* as) {
    if (!as->promo.visible) return;
    SDL_Renderer* r = as->renderer;

    // Dim background
    draw_rect(r, SDL_FRect{0, 0, (float)WINDOW_W, (float)WINDOW_H}, 0, 0, 0, 100);

    // Panel at center: three options HORSE, FERZ, WAZIR
    float     W = as->ui.squarePx * 3.2f;
    float     H = as->ui.squarePx * 1.25f;
    SDL_FRect panel{as->ui.boardRect.x + as->ui.boardRect.w / 2 - W / 2,
                    as->ui.boardRect.y + as->ui.boardRect.h / 2 - H / 2, W, H};
    draw_rect(r, panel, 20, 20, 20, 240);
    draw_outline(r, panel, 180, 180, 180, 255, 4.0f);

    // Layout three equal cells inside
    float                    gap = 12.0f;
    float                    cw  = (W - 4 * gap) / 3.0f;
    float                    ch  = H - 2 * gap;
    std::array<PieceType, 3> order{HORSE, FERZ, WAZIR};

    for (int i = 0; i < 3; ++i) {
        SDL_FRect rc{panel.x + gap + i * (cw + gap), panel.y + gap, cw, ch};
        as->promo.rects[i] = rc;

        // Does an option exist for this piece?
        bool exists = false;
        for (auto m : as->promo.options) {
            if (m.type_of() == PROMOTION && m.promotion_type() == order[i]) {
                exists = true;
                break;
            }
        }

        // Draw
        draw_rect(r, rc, exists ? 60 : 40, exists ? 60 : 40, exists ? 60 : 40, 255);
        Color  who = as->pos.side_to_move();  // promotion side equals mover
        TexKey tk  = texkey_for_piece(who, order[i]);
        if (exists && as->texturesLoaded && as->textures[tk])
            draw_piece_texture(r, as->textures[tk], rc);
        else if (exists)
            draw_piece_fallback(r, who, order[i], rc);

        draw_outline(r, rc, exists ? 200 : 80, exists ? 200 : 80, 30, 255, 3.0f);
    }
}

static void draw_start_screen(AppState* as) {
    // Simple instruction blocks (no text rendering; use color hints)
    SDL_Renderer* r = as->renderer;
    draw_rect(r, SDL_FRect{0, 0, (float)WINDOW_W, (float)WINDOW_H}, 16, 16, 28, 255);
    // Two large buttons-ish areas:
    SDL_FRect left{WINDOW_W * 0.15f, WINDOW_H * 0.25f, WINDOW_W * 0.3f, WINDOW_H * 0.5f};
    SDL_FRect right{WINDOW_W * 0.55f, WINDOW_H * 0.25f, WINDOW_W * 0.3f, WINDOW_H * 0.5f};
    draw_rect(r, left, 220, 220, 230, 255);
    draw_rect(r, right, 30, 30, 40, 255);
    draw_outline(r, left, 0, 0, 0, 255, 6.0f);
    draw_outline(r, right, 255, 255, 255, 255, 6.0f);

    // Icons: white king on left, black king on right (if textures)
    float pad = 40.0f;
    if (as->texturesLoaded) {
        SDL_FRect lrc{left.x + pad, left.y + pad, left.w - 2 * pad, left.h - 2 * pad};
        SDL_FRect rrc{right.x + pad, right.y + pad, right.w - 2 * pad, right.h - 2 * pad};
        draw_piece_texture(r, as->textures[T_W_K], lrc);
        draw_piece_texture(r, as->textures[T_B_K], rrc);
    }
    // (Press W or B – no text rendering; rely on readme/comments)
}

// ---------- Game mechanics ----------
static void apply_move_and_advance(AppState* as, Move m) {
    as->states.emplace_back();
    as->pos.do_move(m, as->states.back());
    as->lastMove.from = m.from_sq();
    as->lastMove.to   = m.to_sq();
    as->selectedSq.reset();
    as->selectedDropPiece.reset();
    as->promo.visible = false;
}

static void start_ai_thinking_if_needed(AppState* as) {
    if (as->phase != Phase::Playing) return;
    if (as->pos.side_to_move() == as->humanSide) return;
    if (as->ai.thinking) return;

    as->ai.thinking = true;
    // Launch async search to keep UI responsive
    as->ai.fut = std::async(std::launch::async, [&]() {
        SearchResult res = search_best_move(as->pos, as->searchDepth);
        return res;
    });
}

static void maybe_finish_ai(AppState* as) {
    if (!as->ai.thinking) return;
    using namespace std::chrono_literals;
    if (as->ai.fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        SearchResult res = as->ai.fut.get();
        as->ai.thinking  = false;
        if (res.bestMove != MOVE_NONE) {
            apply_move_and_advance(as, res.bestMove);
        }
    }
}

static void restart_to_side_select(AppState* as) {
    as->phase       = Phase::SideSelect;
    as->ai.thinking = false;
    as->selectedSq.reset();
    as->selectedDropPiece.reset();
    as->promo.visible = false;
    as->lastMove.from.reset();
    as->lastMove.to.reset();
}

// ---------- Event handling ----------
static bool click_in_pocket(AppState* as, float mx, float my, Color& pocketColorOut,
                            PieceType& ptOut) {
    // Mirror of draw_pockets; compute the same rectangles and hit-test them.
    const SDL_FRect& U    = as->ui.leftUIRect;
    float            pad  = 12.0f;
    float            cell = std::min((U.w - 2 * pad) / 4.0f, (U.h - 3 * pad) / 4.0f);
    float            tH   = cell * 2 + pad;
    SDL_FRect        blackPanel{U.x + pad, U.y + pad, U.w - 2 * pad, tH};
    SDL_FRect        whitePanel{U.x + pad, U.y + U.h - pad - tH, U.w - 2 * pad, tH};

    auto testPanel = [&](Color c, const SDL_FRect& panel) -> bool {
        std::array<PieceType, 4> order{PAWN, HORSE, FERZ, WAZIR};
        float                    gap = cell * 0.2f;
        for (int i = 0; i < 4; ++i) {
            int cnt = as->pos.pocket(c).count(order[i]);
            for (int k = 0; k < std::min(cnt, 2); ++k) {
                SDL_FRect rc{panel.x + i * (cell + gap), panel.y + (k == 0 ? gap : gap * 2 + cell),
                             cell, cell};
                if (point_in_rect(mx, my, rc)) {
                    // Only allow selecting from mover's pocket
                    if (c == as->pos.side_to_move() && cnt > 0) {
                        pocketColorOut = c;
                        ptOut          = order[i];
                        return true;
                    }
                    return false;
                }
            }
        }
        return false;
    };

    if (testPanel(BLACK, blackPanel)) return true;
    if (testPanel(WHITE, whitePanel)) return true;
    return false;
}

static void handle_board_click(AppState* as, float mx, float my) {
    if (as->phase != Phase::Playing) return;
    if (as->pos.side_to_move() != as->humanSide) return;  // wait for AI

    // First, promotion overlay?
    if (as->promo.visible) {
        for (int i = 0; i < 3; ++i) {
            if (point_in_rect(mx, my, as->promo.rects[i])) {
                // Identify promotion piece type by rect index
                std::array<PieceType, 3> order{HORSE, FERZ, WAZIR};
                PieceType                want = order[i];
                for (auto m : as->promo.options) {
                    if (m.promotion_type() == want) {
                        apply_move_and_advance(as, m);
                        // Check terminal or start AI
                        bool  isMate = false, isThree = false;
                        Color win = WHITE;
                        if (is_terminal(as->pos, isMate, win, isThree)) {
                            as->phase             = Phase::GameOver;
                            as->winner            = isMate ? win : win;  // same var
                            as->gameOverCheckmate = isMate;
                        } else {
                            start_ai_thinking_if_needed(as);
                        }
                        return;
                    }
                }
            }
        }
        // click outside: cancel
        as->promo.visible = false;
        return;
    }

    // Pocket click?
    Color     pc;
    PieceType pt;
    if (click_in_pocket(as, mx, my, pc, pt)) {
        if (pc == as->pos.side_to_move()) {
            as->selectedDropPiece = pt;
            as->selectedSq.reset();
        }
        return;
    }

    // Board click
    Square s = screen_to_square(as, mx, my);
    if (s == SQ_NONE) {
        as->selectedSq.reset();
        as->selectedDropPiece.reset();
        return;
    }

    // If a drop piece is selected, try to drop here
    if (as->selectedDropPiece) {
        auto m = find_drop_to(as->pos, *as->selectedDropPiece, s);
        if (m) {
            apply_move_and_advance(as, *m);
            bool  isMate = false, isThree = false;
            Color win = WHITE;
            if (is_terminal(as->pos, isMate, win, isThree)) {
                as->phase             = Phase::GameOver;
                as->winner            = isMate ? win : win;
                as->gameOverCheckmate = isMate;
            } else {
                start_ai_thinking_if_needed(as);
            }
            return;
        } else {
            // clicking elsewhere: clear drop selection or treat as select piece on board
            as->selectedDropPiece.reset();
            // continue to possibly select a board piece
        }
    }

    // If no source selected yet and clicked own piece -> select
    if (!as->selectedSq) {
        if (is_own_piece(as->pos, s)) {
            as->selectedSq = s;
            return;
        } else {
            // clicked empty or opponent; ignore
            return;
        }
    }

    // Have a source; attempt to move to clicked target
    auto candidates = filter_moves_from_to(as->pos, *as->selectedSq, s);
    if (candidates.empty()) {
        // If clicked another own piece, switch selection
        if (is_own_piece(as->pos, s)) {
            as->selectedSq = s;
        } else {
            as->selectedSq.reset();
        }
        return;
    }

    if (candidates.size() == 1) {
        apply_move_and_advance(as, candidates[0]);
    } else {
        // Multiple moves means promotion choices. Show chooser.
        as->promo.from    = *as->selectedSq;
        as->promo.to      = s;
        as->promo.options = candidates;  // all promotion variants
        as->promo.visible = true;
        return;
    }

    // After applying a move, check terminal or start AI.
    bool  isMate = false, isThree = false;
    Color win = WHITE;
    if (is_terminal(as->pos, isMate, win, isThree)) {
        as->phase             = Phase::GameOver;
        as->winner            = isMate ? win : win;  // stalemate winner is side to move per rules
        as->gameOverCheckmate = isMate;
    } else {
        start_ai_thinking_if_needed(as);
    }
}

// ---------- SDL Callbacks ----------
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    if (!SDL_SetAppMetadata("TinyHouse variant of Chess game with AI", "1.0",
                            "com.example.tinyhouse")) {
        return SDL_APP_FAILURE;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    AppState* as = (AppState*)SDL_calloc(1, sizeof(AppState));
    if (!as) return SDL_APP_FAILURE;
    *appstate = as;

    if (!SDL_CreateWindowAndRenderer("TinyHouse Chess", WINDOW_W, WINDOW_H,
                                     SDL_WINDOW_ALWAYS_ON_TOP, &as->window, &as->renderer)) {
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(as->renderer, WINDOW_W, WINDOW_H,
                                     SDL_LOGICAL_PRESENTATION_LETTERBOX);

    // Engine init
    Bitboards::init();
    Position::init();

    as->states.clear();
    as->states.emplace_back();
    as->pos.set(START_FEN, &as->states.back());

    load_all_textures(as);

    as->phase        = Phase::SideSelect;
    as->boardFlipped = false;
    as->humanSide    = WHITE;
    as->lastTicks    = SDL_GetTicks();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    AppState* as = (AppState*)appstate;
    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;

        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            float mx = (float)event->button.x;
            float my = (float)event->button.y;

            if (as->phase == Phase::SideSelect) {
                // No text/UI hit-testing; rely on keyboard W/B.
                // But allow clicking left/right halves for convenience.
                if (mx < WINDOW_W / 2) {
                    as->humanSide = WHITE;
                } else {
                    as->humanSide = BLACK;
                }
                // Reset and start game with chosen side
                as->states.clear();
                as->states.emplace_back();
                as->pos.set(START_FEN, &as->states.back());
                as->boardFlipped = false;
                as->selectedSq.reset();
                as->selectedDropPiece.reset();
                as->promo.visible = false;
                as->lastMove.from.reset();
                as->lastMove.to.reset();
                as->phase = Phase::Playing;

                // If AI to move first, start thinking
                if (as->pos.side_to_move() != as->humanSide) {
                    start_ai_thinking_if_needed(as);
                }
            } else if (as->phase == Phase::Playing || as->phase == Phase::PromotionPick) {
                handle_board_click(as, mx, my);
            } else if (as->phase == Phase::GameOver) {
                // click anywhere -> restart to side select
                restart_to_side_select(as);
            }
            break;
        }
        default:
            break;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    AppState* as = (AppState*)appstate;

    // AI progression
    if (as->phase == Phase::Playing) {
        // Terminal check (constant time, tiny board)
        bool  isMate = false, isThree = false;
        Color win = WHITE;
        if (is_terminal(as->pos, isMate, win, isThree)) {
            as->phase             = Phase::GameOver;
            as->winner            = win;
            as->gameOverCheckmate = isMate;
        } else {
            maybe_finish_ai(as);
        }
    }

    // Render
    SDL_SetRenderDrawColor(as->renderer, 22, 22, 30, 255);
    SDL_RenderClear(as->renderer);

    if (as->phase == Phase::SideSelect) {
        draw_start_screen(as);
    } else {
        draw_board(as);
        draw_last_move(as);
        draw_check(as);
        draw_pieces(as);
        draw_selection(as);
        draw_pockets(as);

        // Promotion overlay (if active)
        draw_promotion_overlay(as);

        // Optionally, “AI thinking” curtain
        if (as->ai.thinking) {
            draw_rect(as->renderer,
                      SDL_FRect{as->ui.leftUIRect.x, as->ui.leftUIRect.y, as->ui.leftUIRect.w, 36},
                      40, 40, 70, 200);
        }

        if (as->phase == Phase::GameOver) {
            // Dim screen
            draw_rect(as->renderer, SDL_FRect{0, 0, (float)WINDOW_W, (float)WINDOW_H}, 0, 0, 0,
                      140);
            // Simple highlight in center (no text; click to restart)
            SDL_FRect box{(float)WINDOW_W / 2 - 200, (float)WINDOW_H / 2 - 100, 400, 200};
            draw_rect(as->renderer, box, 50, 50, 80, 255);
            draw_outline(as->renderer, box, 200, 200, 220, 255, 6.0f);
        }
    }

    SDL_RenderPresent(as->renderer);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    if (!appstate) return;
    AppState* as = (AppState*)appstate;

    for (auto& t : as->textures)
        if (t) SDL_DestroyTexture(t);
    if (as->renderer) SDL_DestroyRenderer(as->renderer);
    if (as->window) SDL_DestroyWindow(as->window);
    SDL_free(as);
}
