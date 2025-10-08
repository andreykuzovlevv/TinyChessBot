#ifndef MOVEGEN_H_INCLUDED
#define MOVEGEN_H_INCLUDED

#include <algorithm>  // IWYU pragma: keep
#include <cstddef>

#include "types.h"

namespace tiny {

class Position;

enum GenType { EVASIONS, NON_EVASIONS, LEGAL };

struct ExtMove : public Move {
    int value;

    void operator=(Move m) { data = m.raw(); }

    // Inhibit unwanted implicit conversions to Move
    // with an ambiguity that yields to a compile error.
    operator float() const = delete;
};

inline bool operator<(const ExtMove& f, const ExtMove& s) { return f.value < s.value; }

template <GenType>
Move* generate(const Position& pos, Move* moveList);

// The MoveList struct wraps the generate() function and returns a convenient
// list of moves. Using MoveList is sometimes preferable to directly calling
// the lower level generate() function.
template <GenType T>
struct MoveList {
    explicit MoveList(const Position& pos) : last(generate<T>(pos, moveList)) {}
    const Move* begin() const { return moveList; }
    const Move* end() const { return last; }
    size_t      size() const { return last - moveList; }
    bool        contains(Move move) const { return std::find(begin(), end(), move) != end(); }

    const Move& operator[](size_t i) const {
        assert(i < size());
        return moveList[i];
    }

   private:
    Move moveList[MAX_MOVES], *last;
};

}  // namespace tiny

#endif  // #ifndef MOVEGEN_H_INCLUDED
