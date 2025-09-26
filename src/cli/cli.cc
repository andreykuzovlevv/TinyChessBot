// cli.cc
// Minimal CLI dispatcher: only two modes.
//   tinyhouse solve --out <file>
//   tinyhouse play --tb <file>

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace
{

    // ----- Usage -----
    void print_usage()
    {
        std::cout <<
            R"(tinyhouse [solve|play] [options]

Commands:

  solve
    --out <path>   (required) output tablebase file

  play
    --tb <path>    (required) load tablebase file

Examples:
  tinyhouse solve --out tinyhouse.tb
  tinyhouse play --tb tinyhouse.tb
)" << std::endl;
    }

    void print_play_repl_help()
    {
        std::cout <<
            R"(REPL commands (stubs):
  startpos
  position thfen <string>
  bestmove
  perft <N>
  d
  quit
)" << std::endl;
    }

    // ----- Command runners (stubs) -----
    int run_solve(const std::string &out_path)
    {
        if (out_path.empty())
        {
            std::cerr << "error: --out is required for 'solve'\n";
            return 2;
        }
        std::cout << "[solve] out=" << out_path << "\n";
        std::cout << "TODO: solver not implemented. This is the entrypoint.\n";
        return 0;
    }

    int run_play(const std::string &tb_path)
    {
        if (tb_path.empty())
        {
            std::cerr << "error: --tb is required for 'play'\n";
            return 2;
        }
        std::cout << "[play] tb=" << tb_path << "\n";
        std::cout << "TODO: engine not implemented. Entering stub REPL.\n";
        print_play_repl_help();

        std::string line;
        std::cout << "tinyhouse> " << std::flush;
        while (std::getline(std::cin, line))
        {
            if (line == "quit" || line == "exit")
                break;
            else if (line == "help" || line == "?")
                print_play_repl_help();
            else if (line == "startpos")
                std::cout << "(startpos) OK [stub]\n";
            else if (line.rfind("position", 0) == 0)
                std::cout << "(position) OK [stub]\n";
            else if (line == "bestmove")
                std::cout << "bestmove (unimplemented)\n";
            else if (line.rfind("perft", 0) == 0)
                std::cout << "(perft) = 0 [stub]\n";
            else if (line == "d")
                std::cout << "(display) [stub]\n";
            else if (line.empty())
            { /* ignore */
            }
            else
                std::cout << "unknown command: " << line << " (type 'help')\n";

            std::cout << "tinyhouse> " << std::flush;
        }
        return 0;
    }

    int cmd_solve(int argc, char **argv)
    {
        if (argc != 2 || std::strcmp(argv[0], "--out") != 0)
        {
            std::cerr << "usage: tinyhouse solve --out <path>\n";
            return 2;
        }
        std::string out_path = argv[1];
        return run_solve(out_path);
    }

    int cmd_play(int argc, char **argv)
    {
        if (argc != 2 || std::strcmp(argv[0], "--tb") != 0)
        {
            std::cerr << "usage: tinyhouse play --tb <path>\n";
            return 2;
        }
        std::string tb_path = argv[1];
        return run_play(tb_path);
    }

} // namespace

// ----- Public entrypoint -----
int run_cli(int argc, char **argv)
{
    if (argc < 2)
    {
        print_usage();
        return 1;
    }

    std::string cmd = argv[1];

    // Shift argv to only sub-args
    std::vector<char *> subargs;
    for (int i = 2; i < argc; ++i)
        subargs.push_back(argv[i]);
    int subargc = static_cast<int>(subargs.size());
    char **subargv = subargc ? subargs.data() : nullptr;

    if (cmd == "solve")
    {
        return cmd_solve(subargc, subargv);
    }
    else if (cmd == "play")
    {
        return cmd_play(subargc, subargv);
    }
    else if (cmd == "help" || cmd == "-h" || cmd == "--help")
    {
        print_usage();
        return 0;
    }
    else
    {
        std::cerr << "error: unknown command '" << cmd << "'\n";
        print_usage();
        return 1;
    }
}
