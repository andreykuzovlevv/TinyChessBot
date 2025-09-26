#pragma once
#include <string>

// Top-level solver entrypoint.
// Given an output path, computes the full Tinyhouse solution
// and writes it to disk.
int solve(const std::string &out_path);
