#include <catch2/catch_test_macros.hpp>
#include "dialogue/PatternCorrelator.h"
#include "utils/PatternUtils.h"
#include <cstdio>
#include <fstream>
#include <memory>

// Helper para crear archivo temporal
std::string tempDbPath() {
    char tmpname[L_tmpnam];
    std::tmpnam(tmpname);
    return std::string(tmpname) + ".db";
}

TEST_CASE("PatternCorrelator: registrar y consultar correlaciones", "[pattern_correlator]") {
    std::string dbPath = tempDbPath();
    {
        PatternCorrelator pc(dbPath);
        Pattern prev = {{"gato", 1.0f}};
        Pattern next = {{"duerme", 1.0f}};
        pc.record("el", prev, next, 1.0f);

        std::vector<std::pair<Pattern, double>> outcomes;
        REQUIRE(pc.query("el", prev, outcomes));
        REQUIRE(outcomes.size() == 1);
        CHECK(outcomes[0].first.begin()->first == "duerme");
        CHECK(outcomes[0].second == 1.0);
    }
    std::remove(dbPath.c_str());
}

TEST_CASE("PatternCorrelator::learnFromText genera trigramas", "[pattern_correlator]") {
    std::string dbPath = tempDbPath();
    {
        PatternCorrelator pc(dbPath);
        pc.learnFromText("el gato duerme la siesta", 1);

        Pattern prev = {{"el", 1.0f}};
        std::vector<std::pair<Pattern, double>> outcomes;
        REQUIRE(pc.query("gato", prev, outcomes));
        bool found = false;
        for (auto& o : outcomes)
            if (o.first.begin()->first == "duerme") found = true;
        CHECK(found);
    }
    std::remove(dbPath.c_str());
}

TEST_CASE("PatternCorrelator: consulta sin datos devuelve falso", "[pattern_correlator]") {
    std::string dbPath = tempDbPath();
    {
        PatternCorrelator pc(dbPath);
        Pattern prev = {{"nada", 1.0f}};
        std::vector<std::pair<Pattern, double>> out;
        CHECK_FALSE(pc.query("algo", prev, out));
    }
    std::remove(dbPath.c_str());
}

TEST_CASE("PatternCorrelator: patron vacío se maneja correctamente", "[pattern_correlator]") {
    std::string dbPath = tempDbPath();
    {
        PatternCorrelator pc(dbPath);
        Pattern empty;
        pc.record("word", empty, empty, 0.0f);
        // no debe crashear
        SUCCEED("No crash");
    }
    std::remove(dbPath.c_str());
}
