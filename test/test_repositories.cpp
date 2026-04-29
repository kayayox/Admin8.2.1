#include <catch2/catch_test_macros.hpp>
#include "db/db_manager.hpp"
#include "db/word_repository.hpp"
#include "db/sentence_repository.hpp"
#include "db/pattern_repository.hpp"
#include "db/dialogue_repository.hpp"
#include "utils/tag_stats.hpp"
#include "core/word.hpp"
#include "dialogue/pattern.hpp"
#include "dialogue/dialogue.hpp"
#include <vector>
#include <optional>

// Fixture – en cada TEST_CASE se resetea la BD a memoria
struct RepoFixture {
    RepoFixture() {
        Database::instance().close();
        Database::instance().init(":memory:");
        TagStats::initTable();
        TagStats::loadDefaultFromStatic();
    }
    ~RepoFixture() { Database::instance().close(); }
};

TEST_CASE_METHOD(RepoFixture, "WordRepository guarda y carga palabra completa", "[word_repository]") {
    Word w("casa");
    w.setSignificado("lugar donde vivir");
    w.setTipo(TipoPalabra::SUST);
    w.setCantidad(Cantidad::SING);
    w.setGenero(Genero::FEME);
    w.setConfianza(0.9f);
    w.save();

    Word loaded;
    REQUIRE(WordRepository::load("casa", loaded));
    CHECK(loaded.getPalabra() == "casa");
    CHECK(loaded.getTipo() == TipoPalabra::SUST);
    CHECK(loaded.getSignificado() == "lugar donde vivir");
}

TEST_CASE_METHOD(RepoFixture, "WordRepository actualiza palabra existente", "[word_repository]") {
    Word w("sol");
    w.setTipo(TipoPalabra::SUST);
    w.save();

    w.setTipo(TipoPalabra::VERB); // cambio simbólico
    w.save();

    Word loaded;
    WordRepository::load("sol", loaded);
    CHECK(loaded.getTipo() == TipoPalabra::VERB);
}

TEST_CASE_METHOD(RepoFixture, "SentenceRepository guarda y carga oración con bloques", "[sentence_repository]") {
    Word w1("el"); w1.setTipo(TipoPalabra::ART);
    Word w2("gato"); w2.setTipo(TipoPalabra::SUST);
    std::vector<Word> words = {w1, w2};
    Oracion oracion(words);
    REQUIRE(oracion.getNumBloques() == 2);
    oracion.save();
    int id = oracion.getId();
    REQUIRE(id > 0);

    Oracion loaded = SentenceRepository::loadById(id);
    REQUIRE(loaded.getNumBloques() == 2);
    CHECK(loaded.getBloques()[0].block == "el");
    CHECK(loaded.getBloques()[1].typo_b == TipoPalabra::SUST);
}

TEST_CASE_METHOD(RepoFixture, "SentenceRepository::loadByKey encuentra oración", "[sentence_repository]") {
    Word w("perro"); w.setTipo(TipoPalabra::SUST);
    std::vector<Word> words = {w};
    Oracion oracion(words);
    oracion.save();

    Oracion found = SentenceRepository::loadByKey("perro", TipoPalabra::SUST);
    CHECK(found.getId() == oracion.getId());
}

TEST_CASE_METHOD(RepoFixture, "PatternRepository guarda y recupera patrón", "[pattern_repository]") {
    Patron p({TipoPalabra::ART, TipoPalabra::SUST}, TipoPatron::AFIRMACION_SIMP);
    PatternRepository::save(p);

    float sim = 0.0f;
    auto match = PatternRepository::findMatch({TipoPalabra::ART, TipoPalabra::SUST}, sim);
    REQUIRE(match.has_value());
    CHECK(match->tipo == TipoPatron::AFIRMACION_SIMP);
    CHECK(sim == 1.0f);

    auto all = PatternRepository::loadAll();
    CHECK(all.size() >= 1);
}

TEST_CASE_METHOD(RepoFixture, "DialogueRepository guarda diálogo y recupera historial", "[dialogue_repository]") {
    std::vector<Word> words;
    Word w1("el"); w1.setTipo(TipoPalabra::ART);
    words.push_back(w1);
    Word w2("perro"); w2.setTipo(TipoPalabra::SUST);
    words.push_back(w2);
    Oracion prem(words);
    prem.save();

    Oracion hip = prem; // simplificación
    hip.save();

    DialogueRepository::saveDialogue(prem, hip, TipoPatron::AFIRMACION_SIMP, 0.8f);

    DialogueHistory history = DialogueRepository::loadHistory();
    REQUIRE(!history.getHistory().empty());
    CHECK(history.getHistory().front().creatividad == 0.8f);
}

TEST_CASE_METHOD(RepoFixture, "Feedback registro no causa errores", "[dialogue_repository]") {
    DialogueRepository::registerFeedback("gato", TipoPalabra::SUST, TipoPalabra::VERB, 0);
    SUCCEED("No lanza excepción");
}

TEST_CASE_METHOD(RepoFixture, "buildCorpus recoge palabras de bloques", "[dialogue_repository]") {
    std::vector<Word> words;
    Word w1("la"); w1.setTipo(TipoPalabra::ART);
    words.push_back(w1);
    Word w2("casa"); w2.setTipo(TipoPalabra::SUST);
    words.push_back(w2);
    Oracion oracion(words);
    oracion.save();
    auto corpus = DialogueRepository::buildCorpus();
    CHECK(corpus.size() == 2);
    CHECK(corpus[0] == "la");
}

TEST_CASE_METHOD(RepoFixture, "TagStats actualiza y consulta probabilidades", "[tag_stats]") {
    TagStats::updateUnigram(TipoPalabra::ART, TipoPalabra::SUST, 10);
    auto probs = TagStats::getUnigramProbs(TipoPalabra::ART);
    REQUIRE(!probs.empty());
    bool found = false;
    for (auto& p : probs)
        if (p.first == TipoPalabra::SUST) found = true;
    CHECK(found);
}
