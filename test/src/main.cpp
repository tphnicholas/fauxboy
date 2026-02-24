#include <catch2/catch_session.hpp>

#include <filesystem>

#include "config.hpp"

std::filesystem::path Config::SINGLE_STEP_TESTS_DIR = ".";

int main(int argc, char* argv[])
{
    Catch::Session session;

    auto cli = session.cli()
               | Catch::Clara::Opt(Config::SINGLE_STEP_TESTS_DIR, "single-step-tests-dir")["--single-step-tests-dir"](
                   "the root directory that contains the json test files");

    session.cli(cli);

    int errorCode = session.applyCommandLine(argc, argv);
    if (errorCode != 0)
    {
        return errorCode;
    }

    return session.run();
}