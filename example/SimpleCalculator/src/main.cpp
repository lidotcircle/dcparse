#include "scalc/lexer_parser.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
using namespace std;


static void usage(string cmd)
{
    auto pos = cmd.find_last_of("/\\");
    if (pos != string::npos)
        cmd = cmd.substr(pos + 1);

    cout << "Usage: " << cmd << " statements " << endl
         << "       " << cmd << " -f <file>" << endl
         << "       " << cmd << endl;
}

static string trimstring(string str)
{
    auto p1 = str.find_first_not_of(" \t");
    if (p1 != string::npos)
        str = str.substr(p1);

    auto p2 = str.find_last_not_of(" \t");
    if (p2 != string::npos)
        str = str.substr(0, p2 + 1);

    return str;
}

int main(int argc, char* argv[])
{
    if (argc > 3) {
        usage(argv[0]);
        return 1;
    }

    string modulename = "";
    string input;
    if (argc == 3) {
        if (string(argv[1]) == "-f") {
            ifstream ifs(argv[2]);
            if (!ifs) {
                cerr << "File not found: " << argv[2] << endl;
                return 1;
            }
            stringstream ss;
            ss << ifs.rdbuf();
            input = ss.str();
            if (input.empty())
                input = ";";
        } else if (string(argv[1]) == "-c") {
            modulename = argv[2];
            ifstream ifs(argv[2]);
            if (!ifs) {
                cerr << "File not found: " << argv[2] << endl;
                return 1;
            }
            stringstream ss;
            ss << ifs.rdbuf();
            input = ss.str();
            if (input.empty())
                input = ";";
        } else {
            usage(argv[0]);
            return 1;
        }
    } else if (argc == 2) {
        input = trimstring(argv[1]);
        if (input.empty()) {
            usage(argv[0]);
            return 1;
        }

        if (string("};").find(input.back()) == string::npos)
            input += ';';
    }

    CalcLexerParser exec(modulename.empty());
    auto ctx = exec.getContext();
    ctx->set_output(&cout);
    const std::string preamble = "function sin(x);"
                                 "function cos(x);";
    if (!input.empty()) {
        input = preamble + input;
        try {
            for (auto c : input)
                exec.feed(c);

            exec.end();
            if (!modulename.empty()) {
                const auto ir = exec.genllvm(modulename);
                const auto pos = modulename.find_last_of('.');
                const auto of = modulename.substr(0, pos) + ".ll";
                std::fstream fs(of, std::ios::binary | std::ios::out);
                fs.write(ir.c_str(), ir.size());
                if (fs.bad())
                    cerr << "fail write to file: " << std::endl << ir << std::endl;
            }
            return 0;
        } catch (const runtime_error& e) {
            cerr << "Error: " << e.what() << endl;
            return 1;
        }
    }

    assert(argc == 1);
    string line;
    cout << "> ";
    while (getline(cin, line)) {
        line = trimstring(line) + "\n\n";

        try {
            for (auto c : line)
                exec.feed(c);
        } catch (const std::runtime_error& e) {
            cerr << "Error: " << e.what() << endl;
            exec.reset();
        }
        cout << "> ";
    }
    exec.end();

    return 0;
}
