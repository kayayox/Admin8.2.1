#include <catch2/catch_test_macros.hpp>
#include "nlp_api.h"
#include "db/db_manager.hpp"
#include <cstdio>
#include <string>

// Fixture que crea dos bases de datos temporales
struct IntegrationFixture {
    std::string semPath, corrPath;
    NLPEngine engine;

    IntegrationFixture() {
        char tmp1[L_tmpnam], tmp2[L_tmpnam];
        std::tmpnam(tmp1); semPath = std::string(tmp1) + "_sem.db";
        std::tmpnam(tmp2); corrPath = std::string(tmp2) + "_corr.db";
        REQUIRE(engine.initialize(semPath, corrPath));
    }

    ~IntegrationFixture() {
        engine.shutdown();
        std::remove(semPath.c_str());
        std::remove(corrPath.c_str());
    }
};

TEST_CASE_METHOD(IntegrationFixture, "Aprender y clasificar una oración simple", "[integration]") {
    engine.learnText("El sol brilla.");
    auto infos = engine.classifySentence("El sol brillante.");
    REQUIRE(infos.size() == 3);
    CHECK(infos[0].tipo == "Artículo");
    CHECK(infos[1].tipo == "Sustantivo");
    CHECK(infos[2].tipo == "Adjetivo");
}

TEST_CASE_METHOD(IntegrationFixture, "Predicción de siguiente palabra", "[integration]") {
    engine.learnText("El gato duerme la siesta");
    auto preds = engine.predictNextWordWithOnePrev("duerme", "gato");
    REQUIRE(!preds.empty());
    CHECK(preds[0].word == "la");
    CHECK(preds[0].probability > 0.0);
}

TEST_CASE_METHOD(IntegrationFixture, "Generación de hipótesis", "[integration]") {
    engine.learnText("El perro ladra.");
    std::string resp = engine.generateHypothesis("El perro ladra.");
    CHECK(!resp.empty());
    // Con el patrón de afirmación, debería ser una copia o similar
    CHECK(resp.find("perro") != std::string::npos);
}

TEST_CASE_METHOD(IntegrationFixture, "Corrección manual de palabra", "[integration]") {
    engine.learnText("casa");
    engine.correctWord("casa", "Adjetivo"); // forzamos a adjetivo
    WordInfo info = engine.getWordInfo("casa");
    CHECK(info.tipo == "Adjetivo");
}
