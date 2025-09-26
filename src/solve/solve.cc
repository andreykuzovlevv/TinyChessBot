#include "solve.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "../core/position.h" // abstract: provides Position and start position
#include "retro.h"            // retrograde engine API (declared below)
#include "tb_write.h"         // tablebase writer API (declared below)

using namespace tiny;

int solve(const std::string &out_path)
{
    std::cout << "[solve] starting solver...\n";
    std::cout << "output file: " << out_path << "\n";

    // 1) Build initial Tinyhouse position (abstract factory).
    Position start = Position::tinyhouse_start();

    // 2) Run retrograde to compute WDL/DTM/best-move for all reachable positions.
    std::vector<retro::TBRecord> records = retro::build_wdl_dtm(start);

    std::cout << "[solve] positions solved: " << records.size() << "\n";

    // 3) Sort by Zobrist key for deterministic, probe-friendly tablebase.
    std::sort(records.begin(), records.end(),
              [](const retro::TBRecord &a, const retro::TBRecord &b)
              {
                  return a.key < b.key;
              });

    // 4) Write tablebase to disk.
    int rc = tb::write_binary(out_path, records);
    if (rc != 0)
    {
        std::cerr << "[solve] error: failed to write tablebase (rc=" << rc << ")\n";
        return rc;
    }

    std::cout << "[solve] done.\n";
    return 0;
}
