#include <catch2/catch_test_macros.hpp>
#include "nlp/classifier.hpp"
#include "core/word.hpp"
#include "db/db_manager.hpp"
#include "utils/tag_stats.hpp"
#include <vector>

// Fixture para inicializar la BD en memoria y las estadísticas por defecto
struct ClassifierFixture {
    ClassifierFixture() {
        // La base de datos se abre en memoria para aislar pruebas
        Database::instance().close();
        bool ok = Database::instance().init(":memory:");
        REQUIRE(ok);
        TagStats::initTable();
        TagStats::loadDefaultFromStatic();
    }

    ~ClassifierFixture() {
        Database::instance().close();
    }
};

TEST_CASE_METHOD(ClassifierFixture, "Clasificación de oración simple con vocabulario conocido", "[classifier]") {
    Classifier cl;
    std::vector<Word> words = {Word("el"), Word("gato"), Word("negro")};
    cl.classifySentence(words);
    REQUIRE(words.size() == 3);
    CHECK(words[0].getTipo() == TipoPalabra::ART);
    CHECK(words[1].getTipo() == TipoPalabra::SUST);
    CHECK(words[2].getTipo() == TipoPalabra::ADJT);
    CHECK(words[0].getConfianza() > 0.9f);
}

TEST_CASE_METHOD(ClassifierFixture, "Palabra desconocida queda como indefinida pero puede ser corregida", "[classifier]") {
    Classifier cl;
    std::vector<Word> words = {Word("xyz123")};
    cl.classifySentence(words);
    REQUIRE(words.size() == 1);
    CHECK(words[0].getTipo() == TipoPalabra::INDEFINIDO);
    CHECK(words[0].getConfianza() < 0.3f);
}

TEST_CASE_METHOD(ClassifierFixture, "requestCorrection mejora la etiqueta de una palabra", "[classifier]") {
    Classifier cl;
    Word w("corriendo");
    w.setTipo(TipoPalabra::INDEFINIDO);
    w.setConfianza(0.2f);
    // contexto: prev2=ART, prev=SUST, next=ADJT   (aunque sea "el gato corriendo rápido")
    cl.requestCorrection(w, TipoPalabra::ART, TipoPalabra::SUST, TipoPalabra::ADJT, 0.2f);
    // Debería convertirse en verbo
    CHECK(w.getTipo() == TipoPalabra::VERB);
    CHECK(w.getConfianza() > 0.4f);
}

TEST_CASE_METHOD(ClassifierFixture, "classifySentence mantiene alta confianza si ya está bien etiquetada", "[classifier]") {
    Classifier cl;
    Word w("casa");
    w.setTipo(TipoPalabra::SUST);
    w.setConfianza(0.95f);
    std::vector<Word> words = {w};
    cl.classifySentence(words);
    CHECK(words[0].getTipo() == TipoPalabra::SUST);
    CHECK(words[0].getConfianza() >= 0.95f);
}

TEST_CASE_METHOD(ClassifierFixture, "Oración vacía no causa problemas", "[classifier]") {
    Classifier cl;
    std::vector<Word> empty;
    REQUIRE_NOTHROW(cl.classifySentence(empty));
}
