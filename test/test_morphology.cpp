#include <catch2/catch_test_macros.hpp>
#include "nlp/morphology.hpp"
#include "types.hpp"

using namespace morphology;

TEST_CASE("Detección de palabras comunes", "[morphology]") {
    TipoPalabra tag;
    float conf;
    REQUIRE(isCommonWord("casa", tag, conf));
    CHECK(tag == TipoPalabra::SUST);
    CHECK(conf > 0.9f);

    REQUIRE(isCommonWord("bueno", tag, conf));
    CHECK(tag == TipoPalabra::ADJT);

    REQUIRE(isCommonWord("ser", tag, conf));
    CHECK(tag == TipoPalabra::VERB);

    REQUIRE(isCommonWord("nunca", tag, conf));
    CHECK(tag == TipoPalabra::ADV);

    REQUIRE(isCommonWord("el", tag, conf));
    CHECK(tag == TipoPalabra::ART);

    REQUIRE_FALSE(isCommonWord("xyzdesconocida", tag, conf));
}

TEST_CASE("guessInitialTag clasifica correctamente", "[morphology]") {
    CHECK(guessInitialTag("el") == TipoPalabra::ART);
    CHECK(guessInitialTag("de") == TipoPalabra::PREP);
    CHECK(guessInitialTag("y") == TipoPalabra::CONJ);
    CHECK(guessInitialTag("qué") == TipoPalabra::PREG);
    CHECK(guessInitialTag("correr") == TipoPalabra::VERB);
    CHECK(guessInitialTag("rápido") == TipoPalabra::ADJT);
    CHECK(guessInitialTag("casa") == TipoPalabra::SUST);
    CHECK(guessInitialTag("suavemente") == TipoPalabra::ADV);
    CHECK(guessInitialTag("xyzzy") == TipoPalabra::INDEFINIDO);
}

TEST_CASE("validateTag devuelve puntuaciones coherentes", "[morphology]") {
    CHECK(validateTag("casa", TipoPalabra::SUST) > 0.6f);
    CHECK(validateTag("corriendo", TipoPalabra::VERB) > 0.7f);
    CHECK(validateTag("rápido", TipoPalabra::ADJT) > 0.6f);
    CHECK(validateTag("suavemente", TipoPalabra::ADV) > 0.7f);
    CHECK(validateTag("qué", TipoPalabra::PREG) > 0.9f);
    CHECK(validateTag("este", TipoPalabra::DEMS) > 0.8f);
    CHECK(validateTag("dos", TipoPalabra::NUM) > 0.8f);
    CHECK(validateTag("que", TipoPalabra::RELT) > 0.8f);
    CHECK(validateTag("muchos", TipoPalabra::CUANT) > 0.7f);
    // etiqueta inválida para la palabra
    CHECK(validateTag("casa", TipoPalabra::VERB) < 0.2f);
}

TEST_CASE("isPlural funciona con excepciones", "[morphology]") {
    CHECK(isPlural("casas"));
    CHECK_FALSE(isPlural("casa"));
    CHECK_FALSE(isPlural("crisis"));   // excepción
    CHECK_FALSE(isPlural("martes"));
}

TEST_CASE("detectGender asigna según terminación", "[morphology]") {
    CHECK(detectGender("niño") == Genero::MASC);
    CHECK(detectGender("niña") == Genero::FEME);
    CHECK(detectGender("mapa") == Genero::MASC); // excepción
    CHECK(detectGender("mano") == Genero::FEME);
}

TEST_CASE("detectTense reconoce tiempos verbales", "[morphology]") {
    CHECK(detectTense("comió") == Tiempo::PASS);
    CHECK(detectTense("comía") == Tiempo::PASS);
    CHECK(detectTense("comerá") == Tiempo::FUTR);
    CHECK(detectTense("come") == Tiempo::PRES);
}

TEST_CASE("detectPerson identifica personas", "[morphology]") {
    CHECK(detectPerson("como") == Persona::PRIM);
    CHECK(detectPerson("comes") == Persona::SEGU);
    CHECK(detectPerson("come") == Persona::TERC);
    CHECK(detectPerson("comemos") == Persona::PRIM);
    CHECK(detectPerson("comen") == Persona::TERC);
}

TEST_CASE("detectAdjectiveDegree funciona", "[morphology]") {
    CHECK(detectAdjectiveDegree("grandísimo") == Grado::SUPERLA);
    CHECK(detectAdjectiveDegree("mayor") == Grado::COMPARA);
    CHECK(detectAdjectiveDegree("rápido") == Grado::POSIT);
}

TEST_CASE("Funciones de reconocimiento directo", "[morphology]") {
    CHECK(isArticle("un"));
    CHECK_FALSE(isArticle("casa"));
    CHECK(isPreposition("para"));
    CHECK(isConjunction("pero"));
    CHECK(isInterrogative("cómo"));
    CHECK(isDemonstrative("esta"));
    CHECK(isNumeral("cinco"));
    CHECK(isRelativePronoun("que"));
    CHECK(isQuantifier("varios"));
}
