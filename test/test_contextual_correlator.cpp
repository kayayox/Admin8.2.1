#include <catch2/catch_test_macros.hpp>
#include "core/ContextualCorrelator.h"
#include "dialogue/PatternCorrelator.h"
#include <cstdio>
#include <string>

std::string tempDb() {
    char tmp[L_tmpnam];
    std::tmpnam(tmp);
    return std::string(tmp) + ".ctx.db";
}

TEST_CASE("ContextualCorrelator: aprendizaje con dos anteriores y consulta", "[contextual_correlator]") {
    std::string dbPath = tempDb();
    {
        PatternCorrelator pc(dbPath);
        ContextualCorrelator ctx(pc);

        ctx.learnWithPreviousTwo("el gato duerme la siesta");

        std::vector<std::pair<Pattern, double>> outcomes;
        REQUIRE(ctx.queryNextWithTwoPrev("duerme", "gato", "el", outcomes));
        CHECK_FALSE(outcomes.empty());
        CHECK(outcomes[0].first.begin()->first == "la");
    }
    std::remove(dbPath.c_str());
}

TEST_CASE("ContextualCorrelator: aprendizaje directo sin contexto", "[contextual_correlator]") {
    std::string dbPath = tempDb();
    {
        PatternCorrelator pc(dbPath);
        ContextualCorrelator ctx(pc);

        ctx.learnNextWordDirect("perro come hueso");

        std::vector<std::pair<Pattern, double>> out;
        bool found = ctx.queryNext("come", {"__NO_CONTEXT__"}, out);
        CHECK(found);
        CHECK(out[0].first.begin()->first == "hueso");
    }
    std::remove(dbPath.c_str());
}

TEST_CASE("ContextualCorrelator: consulta sin datos devuelve false", "[contextual_correlator]") {
    std::string dbPath = tempDb();
    {
        PatternCorrelator pc(dbPath);
        ContextualCorrelator ctx(pc);
        std::vector<std::pair<Pattern, double>> out;
        CHECK_FALSE(ctx.queryNextWithOnePrev("nada", "nada", out));
    }
    std::remove(dbPath.c_str());
}
