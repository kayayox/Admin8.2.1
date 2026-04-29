#include <catch2/catch_test_macros.hpp>
#include "dialogue/dialogue.hpp"
#include "dialogue/pattern.hpp"
#include "db/sentence_repository.hpp"
#include "core/word.hpp"

TEST_CASE("clasificarOracion identifica patrones", "[dialogue]") {
    CHECK(clasificarOracion({TipoPalabra::ART, TipoPalabra::SUST, TipoPalabra::VERB}) == TipoPatron::AFIRMACION_COMP);
    CHECK(clasificarOracion({TipoPalabra::ART, TipoPalabra::SUST, TipoPalabra::ADV}) == TipoPatron::NEGACION_SIMP); // ADV simula negación (simplificación)
    CHECK(clasificarOracion({TipoPalabra::PREG, TipoPalabra::VERB}) == TipoPatron::PREGUNTA_SIMP);
}

TEST_CASE("generateHypothesis añade negación cuando el patrón es NEGACION_SIMP", "[dialogue]") {
    std::vector<Word> words;
    Word w1("él"); w1.setTipo(TipoPalabra::PRON);
    words.push_back(w1);
    Word w2("come"); w2.setTipo(TipoPalabra::VERB);
    words.push_back(w2);
    Oracion prem(words);
    Patron p({TipoPalabra::PRON, TipoPalabra::VERB}, TipoPatron::NEGACION_SIMP);
    Oracion hyp = generateHypothesis(prem, &p);
    // Debería insertar "no" después del verbo
    auto blocks = hyp.getBloques();
    bool found_no = false;
    for (auto& b : blocks) {
        if (b.block == "no") found_no = true;
    }
    CHECK(found_no);
}

TEST_CASE("computeCreativity retorna valores consistentes", "[dialogue]") {
    std::vector<Word> words;
    Word w1("perro"); w1.setTipo(TipoPalabra::SUST);
    words.push_back(w1);
    Word w2("ladra"); w2.setTipo(TipoPalabra::VERB);
    words.push_back(w2);
    Oracion prem(words);
    Oracion hip = prem;
    Patron p;
    float c = computeCreativity(prem, hip, p);
    CHECK(c >= 0.0f);
    CHECK(c <= 1.0f);
}

TEST_CASE("DialogueHistory calcula umbral de creatividad", "[dialogue]") {
    DialogueHistory hist;
    Oracion dummmy;
    hist.addDialogue(dummmy, dummmy, Patron(), 0.7f);
    hist.addDialogue(dummmy, dummmy, Patron(), 0.9f);
    CHECK(hist.getThresholdCreativity() == 0.8f);
}
