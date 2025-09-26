#pragma once
#include <string>
#include <vector>
#include "solve/retro.h"

namespace tiny::tb {

int write_binary(const std::string& path,
                 const std::vector<retro::TBRecord>& recs);

} // namespace tiny::tb
