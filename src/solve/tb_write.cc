#include "solve/tb_write.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace tiny::tb {

#pragma pack(push, 1)
struct TBHeader {
    char     magic[8];
    uint32_t version;
    uint64_t count;
};
struct TBRow {
    uint64_t key;
    uint8_t  wdl;   // 0=Loss,1=Draw,2=Win
    uint16_t dtm;
    uint32_t move;
};
#pragma pack(pop)

int write_binary(const std::string& path,
                 const std::vector<retro::TBRecord>& recs) {
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return 1;

    TBHeader h{};
    std::memcpy(h.magic, "TNYTB\0\1", 8);
    h.version = 1;
    h.count   = recs.size();

    if (std::fwrite(&h, sizeof(h), 1, f) != 1) { std::fclose(f); return 2; }

    for (const auto& r : recs) {
        TBRow row{};
        row.key  = r.key;
        row.wdl  = static_cast<uint8_t>(r.wdl);
        row.dtm  = r.dtm;
        row.move = static_cast<uint32_t>(r.best);
        if (std::fwrite(&row, sizeof(row), 1, f) != 1) { std::fclose(f); return 3; }
    }
    std::fclose(f);
    return 0;
}

} // namespace tiny::tb
