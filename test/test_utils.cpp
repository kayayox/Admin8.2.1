#include <catch2/catch_test_macros.hpp>
#include "utils/PatternUtils.h"
#include "core/Learning.h"
#include "utils/string_conv.hpp"
#include <string>

TEST_CASE("serializePattern y deserializePattern son inversas", "[utils]") {
    Pattern orig;
    orig["gato"] = 1.0f;
    orig["perro"] = 0.5f;
    std::string ser = serializePattern(orig);
    Pattern back = deserializePattern(ser);
    CHECK(back.size() == 2);
    CHECK(back["gato"] == 1.0f);
    CHECK(back["perro"] == 0.5f);
}

TEST_CASE("serializePattern de patrón vacío", "[utils]") {
    Pattern empty;
    std::string ser = serializePattern(empty);
    Pattern back = deserializePattern(ser);
    CHECK(back.empty());
}

TEST_CASE("eraserSpace elimina espacios iniciales y finales", "[utils]") {
    std::string s = "  hola mundo  ";
    eraserSpace(s);
    CHECK(s == "hola mundo");
}

TEST_CASE("String conversions cubren todos los tipos", "[utils]") {
    // Solo verificamos que no lanzan excepciones
    CHECK_NOTHROW(tipoToString(TipoPalabra::SUST));
    CHECK_NOTHROW(tiempoToString(Tiempo::PASS));
    CHECK_NOTHROW(generoToString(Genero::FEME));
    CHECK_NOTHROW(personaToString(Persona::SEGU));
    CHECK_NOTHROW(gradoToString(Grado::COMPARA));
    CHECK_NOTHROW(cantidadToString(Cantidad::PLUR));
    CHECK_NOTHROW(tipoPatronToString(TipoPatron::PREGUNTA_SIMP));
}
