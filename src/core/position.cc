#include "position.h"

#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string_view>
#include <utility>

#include "bitboard.h"
#include "misc.h"

using std::string;

namespace tiny {

namespace Zobrist {

Key psq[PIECE_NB][SQUARE_NB];
Key side;
}  // namespace Zobrist

namespace {

static constexpr Piece Pieces[] = {W_PAWN, W_HORSE, W_FERZ, W_WAZIR, W_KING,
                                   B_PAWN, B_HORSE, B_FERZ, B_WAZIR, B_KING};

// Piece to character mapping for FEN output
constexpr std::string_view PieceToChar = " PWFUK   pwfuk  ";
}  // namespace

// Returns an ASCII representation of the position
std::ostream& operator<<(std::ostream& os, const Position& pos) {
    os << "\n +---+---+---+---+\n";

    for (Rank r = RANK_4; r >= RANK_1; --r) {
        for (File f = FILE_A; f <= FILE_D; ++f)
            os << " | " << PieceToChar[pos.piece_on(make_square(f, r))];

        os << " | " << (1 + r) << "\n +---+---+---+---+\n";
    }

    os << "   a   b   c   d\n"
       // << "\nFen: " << pos.fen() << "\nKey: " << std::hex << std::uppercase << std::setfill('0')
       << std::setw(16) << pos.key() << std::setfill(' ') << std::dec << "\nCheckers: ";

    // for (Bitboard b = pos.checkers(); b;) os << UCIEngine::square(pop_lsb(b)) << " ";

    return os;
}

// Implements Marcel van Kervinck's cuckoo algorithm to detect repetition of positions
// for 3-fold repetition draws. The algorithm uses two hash tables with Zobrist hashes
// to allow fast detection of recurring positions. For details see:
// http://web.archive.org/web/20201107002606/https://marcelk.net/2013-04-06/paper/upcoming-rep-v2.pdf

// First and second hash functions for indexing the cuckoo tables
inline int H1(Key h) { return h & 0x7ff; }
inline int H2(Key h) { return (h >> 16) & 0x7ff; }

// Cuckoo tables with Zobrist hashes of valid reversible moves, and the moves themselves
std::array<Key, 2048>  cuckoo;
std::array<Move, 2048> cuckooMove;

// Initializes at startup the various arrays used to compute hash keys
void Position::init() {
    PRNG rng(1070372);

    for (Piece pc : Pieces)
        for (Square s = SQ_A1; s <= SQ_D4; ++s) Zobrist::psq[pc][s] = rng.rand<Key>();
    // pawns on these squares will promote
    std::fill_n(Zobrist::psq[W_PAWN] + SQ_A4, 4, 0);
    std::fill_n(Zobrist::psq[B_PAWN] + SQ_A1, 4, 0);

    Zobrist::side = rng.rand<Key>();

    // Prepare the cuckoo tables
    cuckoo.fill(0);
    cuckooMove.fill(Move::none());
    int count = 0;
    for (Piece pc : Pieces)
        for (Square s1 = SQ_A1; s1 <= SQ_D4; ++s1)
            for (Square s2 = Square(s1 + 1); s2 <= SQ_D4; ++s2)
                if ((type_of(pc) != PAWN) && (attacks_bb(type_of(pc), s1, 0) & s2)) {
                    Move move = Move(s1, s2);
                    Key  key  = Zobrist::psq[pc][s1] ^ Zobrist::psq[pc][s2] ^ Zobrist::side;
                    int  i    = H1(key);
                    while (true) {
                        std::swap(cuckoo[i], key);
                        std::swap(cuckooMove[i], move);
                        if (move == Move::none())  // Arrived at empty slot?
                            break;
                        i = (i == H1(key)) ? H2(key) : H1(key);  // Push victim to alternative slot
                    }
                    count++;
                }
    std::cout << count << " reversible moves in cuckoo hash\n";
}

// Initializes the position object with the given FEN string.
// This function is not very robust - make sure that input FENs are correct,
// this is assumed to be the responsibility of the GUI.
Position& Position::set(const string& fenStr, StateInfo* si) {
    /*
   A FEN string defines a particular position using only the ASCII character set.

   A FEN string contains six fields separated by a space. The fields are:

   1) Piece placement (from white's perspective). Each rank is described, starting
      with rank 8 and ending with rank 1. Within each rank, the contents of each
      square are described from file A through file H. Following the Standard
      Algebraic Notation (SAN), each piece is identified by a single letter taken
      from the standard English names. White pieces are designated using upper-case
      letters ("PNBRQK") whilst Black uses lowercase ("pnbrqk"). Blank squares are
      noted using digits 1 through 8 (the number of blank squares), and "/"
      separates ranks.

   2) Active color. "w" means white moves next, "b" means black.

   3) NO Castling 4) NO En passant

   5) Halfmove clock. This is the number of halfmoves since the last pawn advance
      or capture. This is used to determine if a draw can be claimed under the
      fifty-move rule.

   6) Fullmove number. The number of the full move. It starts at 1, and is
      incremented after Black's move.
*/

    unsigned char      col, row, token;
    size_t             idx;
    Square             sq = SQ_A4;
    std::istringstream ss(fenStr);

    std::memset(this, 0, sizeof(Position));
    std::memset(si, 0, sizeof(StateInfo));
    st = si;

    ss >> std::noskipws;

    // 1. Piece placement
    while ((ss >> token) && !isspace(token)) {
        if (isdigit(token))
            sq += (token - '0') * EAST;  // Advance the given number of files

        else if (token == '/')
            sq += 2 * SOUTH;

        else if ((idx = PieceToChar.find(token)) != string::npos) {
            put_piece(Piece(idx), sq);
            ++sq;
        }
    }
    std::cout << "bb=" << std::bitset<16>(pieces()) << "\n";

    // 2. Active color
    ss >> token;
    sideToMove = (token == 'w' ? WHITE : BLACK);
    ss >> token;

    // 5-6. Halfmove clock and fullmove number
    ss >> std::skipws >> gamePly;

    // Convert from fullmove starting from 1 to gamePly starting from 0,
    // handle also common incorrect FEN with fullmove = 0.
    gamePly = std::max(2 * (gamePly - 1), 0) + (sideToMove == BLACK);
    set_state();

    assert(pos_is_ok());
    std::cout << "hello set func 4\n";

    return *this;
}

// Computes the hash keys of the position, and other
// data that once computed is updated incrementally as moves are made.
// The function is only used when a new position is set up
void Position::set_state() const {
    st->key        = 0;
    st->checkersBB = attackers_to(square<KING>(sideToMove)) & pieces(~sideToMove);

    for (Bitboard b = pieces(); b;) {
        Square s  = pop_lsb(b);
        Piece  pc = piece_on(s);
        st->key ^= Zobrist::psq[pc][s];
    }

    if (sideToMove == BLACK) st->key ^= Zobrist::side;
}

// Computes a bitboard of all pieces which attack a given square.
// Slider attacks use the occupied bitboard to indicate occupancy.
Bitboard Position::attackers_to(Square s, Bitboard occupied) const {
    return (attacks_bb<FERZ>(s) & pieces(FERZ)) | (attacks_bb<WAZIR>(s) & pieces(WAZIR)) |
           (attacks_bb<PAWN>(s, BLACK) & pieces(WHITE, PAWN)) |
           (attacks_bb<PAWN>(s, WHITE) & pieces(BLACK, PAWN)) |
           (attacks_bb<HORSE>(s, occupied) & pieces(HORSE)) | (attacks_bb<KING>(s) & pieces(KING));
}

bool Position::attackers_to_exist(Square s, Bitboard occupied, Color c) const {
    // Pawns: squares from which a pawn of color c would attack s
    if (attacks_bb<PAWN>(s, ~c) & pieces(c, PAWN)) {
        printf("PAWN attacks\n");
        return true;
    }

    // Horse (xiangqi): leg-blocked; needs occupancy
    if (attacks_bb<HORSE>(s, occupied) & pieces(c, HORSE)) {
        printf("HORSE attacks\n");
        return true;
    }

    // Ferz: diagonal king-step (no occupancy needed)
    if (attacks_bb<FERZ>(s) & pieces(c, FERZ)) {
        printf("FERZ attacks\n");
        return true;
    }

    // Wazir: orthogonal king-step (no occupancy needed)
    if (attacks_bb<WAZIR>(s) & pieces(c, WAZIR)) {
        printf("WAZIR attacks\n");
        return true;
    }

    // King: standard king steps (no occupancy needed)
    if (attacks_bb<KING>(s) & pieces(c, KING)) {
        printf("  sq s:\n%s\n", Bitboards::pretty(square_bb(s)).c_str());
        printf("  attacking piece:\n%s\n",
               Bitboards::pretty(attacks_bb<KING>(s) & pieces(c, KING)).c_str());
        printf("  all attacks on sq s for kings:\n%s\n",
               Bitboards::pretty(attacks_bb<KING>(s)).c_str());

        printf("KING attacks\n");
        return true;
    }

    return false;
}

// Returns a FEN representation of the position. In case of
// Chess960 the Shredder-FEN notation is used. This is mainly a debugging function.
string Position::fen() const {
    int                emptyCnt;
    std::ostringstream ss;

    for (Rank r = RANK_4; r >= RANK_1; --r) {
        for (File f = FILE_A; f <= FILE_D; ++f) {
            for (emptyCnt = 0; f <= FILE_D && empty(make_square(f, r)); ++f) ++emptyCnt;

            if (emptyCnt) ss << emptyCnt;

            if (f <= FILE_D) ss << PieceToChar[piece_on(make_square(f, r))];
        }

        if (r > RANK_1) ss << '/';
    }

    ss << (sideToMove == WHITE ? " w " : " b ");

    ss << " " << 1 + (gamePly - (sideToMove == BLACK)) / 2;

    return ss.str();
}

// Performs some consistency checks for the position object
// and raise an assert if something wrong is detected.
// This is meant to be helpful when debugging.
bool Position::pos_is_ok() const {
    constexpr bool Fast = false;  // Quick (default) or full check?

    if ((sideToMove != WHITE && sideToMove != BLACK) || piece_on(square<KING>(WHITE)) != W_KING ||
        piece_on(square<KING>(BLACK)) != B_KING)
        assert(0 && "pos_is_ok: Default");

    if (Fast) return true;

    if (pieceCount[W_KING] != 1 || pieceCount[B_KING] != 1 ||
        attackers_to_exist(square<KING>(~sideToMove), pieces(), sideToMove)) {
        printf("Sq King in check: %d\n", square<KING>(~sideToMove));
        printf("  King in check:\n%s\n",
               Bitboards::pretty(square_bb(square<KING>(~sideToMove))).c_str());

        assert(0 && "pos_is_ok: Kings");
    }

    if (pieceCount[W_PAWN] > 2 || pieceCount[B_PAWN] > 2) assert(0 && "pos_is_ok: Pawns");

    if ((pieces(WHITE) & pieces(BLACK)) || (pieces(WHITE) | pieces(BLACK)) != pieces() ||
        popcount(pieces(WHITE)) > 9 || popcount(pieces(BLACK)) > 9)
        assert(0 && "pos_is_ok: Bitboards");

    for (PieceType p1 = PAWN; p1 <= KING; ++p1)
        for (PieceType p2 = PAWN; p2 <= KING; ++p2)
            if (p1 != p2 && (pieces(p1) & pieces(p2))) assert(0 && "pos_is_ok: Bitboards");

    for (Piece pc : Pieces)
        if (pieceCount[pc] != popcount(pieces(color_of(pc), type_of(pc))) ||
            pieceCount[pc] != std::count(board, board + SQUARE_NB, pc))
            assert(0 && "pos_is_ok: Pieces");
    return true;
}

}  // namespace tiny
