#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <test.hpp>

#include <cstdlib>
#include <fstream>
#include <ios>
#include <iostream>
#include <stdexcept>
#include <string>

bool got_an_assert = false;

static std::string load_test_tile(const std::string& filename) {
    std::ifstream stream{filename, std::ios_base::in | std::ios_base::binary};
    if (!stream.is_open()) {
        throw std::runtime_error{"could not open: '" + filename + "'"};
    }

    const std::string message{std::istreambuf_iterator<char>(stream.rdbuf()),
                              std::istreambuf_iterator<char>()};

    stream.close();
    return message;
}

std::string load_test_tile() {
    return load_test_tile("data/mapbox-streets-v6-14-8714-8017.mvt");
}

std::string load_fixture_tile(const char* env, const std::string& path) {
    const auto fixtures_dir = std::getenv(env);
    if (fixtures_dir == nullptr) {
        std::cerr << "Set "
                  << env
                  << " environment variable to the directory where the mvt fixtures are!\n";
        std::exit(2);
    }

    return load_test_tile(std::string{fixtures_dir} + "/" + path);
}

