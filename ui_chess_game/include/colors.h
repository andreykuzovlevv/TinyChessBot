#include "SDL_render.h"
#include "SDL_stdinc.h"

struct DrawColor {
    Uint8 r, g, b, a;

    // Constructor
    constexpr DrawColor(Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {}

    // Set SDL renderer color
    void set_sdl_color(SDL_Renderer* renderer) const {
        SDL_SetRenderDrawColor(renderer, r, g, b, a);
    }

    // Predefined colors
    static constexpr DrawColor White() { return DrawColor(255, 255, 255); }
    static constexpr DrawColor Black() { return DrawColor(0, 0, 0); }
    static constexpr DrawColor Red() { return DrawColor(255, 0, 0); }
    static constexpr DrawColor Green() { return DrawColor(0, 255, 0); }
    static constexpr DrawColor Blue() { return DrawColor(0, 0, 255); }
    static constexpr DrawColor Yellow() { return DrawColor(255, 255, 0); }
    static constexpr DrawColor Cyan() { return DrawColor(0, 255, 255); }
    static constexpr DrawColor Magenta() { return DrawColor(255, 0, 255); }
    static constexpr DrawColor Gray() { return DrawColor(128, 128, 128); }
    static constexpr DrawColor DarkGray() { return DrawColor(64, 64, 64); }
    static constexpr DrawColor LightGray() { return DrawColor(192, 192, 192); }
    static constexpr DrawColor Transparent() { return DrawColor(0, 0, 0, 0); }
};

// ---------- Predefined Colors ----------
namespace Colors {
// Board colors
constexpr DrawColor BoardLight = DrawColor(196, 172, 132);
constexpr DrawColor BoardDark  = DrawColor(105, 84, 68);

// UI colors
constexpr DrawColor Bright       = DrawColor(240, 240, 240);
constexpr DrawColor Background   = DrawColor(40, 40, 40);
constexpr DrawColor OnBright     = DrawColor(210, 210, 210);
constexpr DrawColor OnBackground = DrawColor(70, 70, 70);
constexpr DrawColor Border       = DrawColor(120, 120, 120);

// Selection colors
constexpr DrawColor SelectionOutline = DrawColor(40, 200, 255);
constexpr DrawColor MoveHint         = DrawColor(0, 0, 0, 50);
constexpr DrawColor MoveHighlight    = DrawColor(0, 200, 0, 60);
constexpr DrawColor CaptureHighlight = DrawColor(255, 160, 0, 60);
constexpr DrawColor DropHighlight    = DrawColor(60, 200, 60, 60);
constexpr DrawColor MoveDot          = DrawColor(0, 0, 0, 180);

// Game state colors
constexpr DrawColor LastMoveFrom   = DrawColor(76, 159, 237);
constexpr DrawColor LastMoveTo     = DrawColor(76, 196, 237);
constexpr DrawColor CheckHighlight = DrawColor(220, 30, 30, 60);

// Promotion colors
constexpr DrawColor PromotionBackground           = DrawColor(0, 0, 0, 100);
constexpr DrawColor PromotionPanel                = DrawColor(20, 20, 20, 240);
constexpr DrawColor PromotionBorder               = DrawColor(180, 180, 180);
constexpr DrawColor PromotionOption               = DrawColor(60, 60, 60);
constexpr DrawColor PromotionOptionBorder         = DrawColor(200, 200, 30);
constexpr DrawColor PromotionOptionDisabled       = DrawColor(40, 40, 40);
constexpr DrawColor PromotionOptionDisabledBorder = DrawColor(80, 80, 30);

// Game over colors
constexpr DrawColor GameOverOverlay = DrawColor(0, 0, 0, 140);
constexpr DrawColor GameOverBox     = DrawColor(50, 50, 80);
constexpr DrawColor GameOverBorder  = DrawColor(200, 200, 220);

// AI thinking indicator
constexpr DrawColor AIThinking = DrawColor(40, 40, 70, 200);

// Pocket colors
constexpr DrawColor PocketBackground = DrawColor(50, 50, 50);
constexpr DrawColor PocketBorder     = DrawColor(100, 100, 100);
constexpr DrawColor CountText        = DrawColor(255, 255, 255);
}  // namespace Colors