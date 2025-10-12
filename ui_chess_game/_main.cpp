#define SDL_MAIN_USE_CALLBACKS 1 /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <optional>

#include "../../src/core/movegen.h"
#include "../../src/core/position.h"
#include "../../src/core/types.h"
#include "../../src/minmax/minmax.h"

#define SDL_WINDOW_WIDTH 1920
#define SDL_WINDOW_HEIGHT 1080

using namespace tiny;

typedef struct {
    // Engine state
    Position              pos;
    std::deque<StateInfo> states;
    std::vector<Move>     history;

    // Session params
    Color humanSide   = WHITE;
    int   searchDepth = 9;
    bool  gameOver    = false;

    // Selection state
    std::optional<Square>    selectedSq;      // currently selected board square
    std::optional<PieceType> selectedDrop;    // currently selected pocket piece to drop
    std::vector<Move>        selectionMoves;  // legal moves for selection

    // Promotion popup context
    bool              wantPromotion = false;
    Square            promoFrom     = SQ_NONE;
    Square            promoTo       = SQ_NONE;
    std::vector<Move> promoChoices;  // candidates that share from->to but different piece

    // UI flags
    bool flipBoard = false;  // if true, side to move at bottom; default false (White bottom)

    // Status
    std::string status;
} ChessContext;

typedef struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    ChessContext  chess_ctx;
} AppState;

SDL_AppResult SDL_AppIterate(void *appstate) {
    AppState     *as  = (AppState *)appstate;
    ChessContext *ctx = &as->chess_ctx;

    SDL_SetRenderDrawColor(as->renderer, 35, 35, 100, 255);
    SDL_RenderClear(as->renderer);
    SDL_RenderPresent(as->renderer);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    if (!SDL_SetAppMetadata("TinyHouse variant of Chess game with ai", "1.0",
                            "com.example.Chess")) {
        return SDL_APP_FAILURE;
    }

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    AppState *as = (AppState *)SDL_calloc(1, sizeof(AppState));
    if (!as) {
        return SDL_APP_FAILURE;
    }

    *appstate = as;

    if (!SDL_CreateWindowAndRenderer("TinyHouse Chess", SDL_WINDOW_WIDTH, SDL_WINDOW_HEIGHT,
                                     SDL_WINDOW_ALWAYS_ON_TOP, &as->window, &as->renderer)) {
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(as->renderer, SDL_WINDOW_WIDTH, SDL_WINDOW_HEIGHT,
                                     SDL_LOGICAL_PRESENTATION_LETTERBOX);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    ChessContext *ctx = &((AppState *)appstate)->chess_ctx;
    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;

        default:
            break;
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    if (appstate != NULL) {
        AppState *as = (AppState *)appstate;
        SDL_DestroyRenderer(as->renderer);
        SDL_DestroyWindow(as->window);
        SDL_free(as);
    }
}
