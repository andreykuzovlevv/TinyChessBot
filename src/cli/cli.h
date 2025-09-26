// cli.h
// Public interface for the command-line driver.

#pragma once

// Entry point for the CLI.
// This function parses arguments and dispatches to subcommands.
// Returns process exit code (0 = success, nonzero = error).
int run_cli(int argc, char **argv);
