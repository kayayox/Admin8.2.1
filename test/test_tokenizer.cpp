#include <catch2/catch_test_macros.hpp>
#include "tokenizer/tokenizer.hpp"
#include <vector>

TEST_CASE("Tokenizer separa palabras simples", "[tokenizer]") {
    auto tokens = tokenize("Hola mundo");
    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].text == "hola");
    CHECK(tokens[0].type == TokenType::WORD);
    CHECK(tokens[1].text == "mundo");
    CHECK(tokens[1].type == TokenType::WORD);
}

TEST_CASE("Tokenizer detecta números", "[tokenizer]") {
    auto tokens = tokenize("42 3.14 -7 0.5");
    REQUIRE(tokens.size() == 4);
    CHECK(tokens[0].type == TokenType::NUMBER);
    CHECK(tokens[0].text == "42");
    CHECK(tokens[1].type == TokenType::NUMBER);
    CHECK(tokens[1].text == "3.14");
    CHECK(tokens[2].type == TokenType::NUMBER);
    CHECK(tokens[2].text == "-7");
    CHECK(tokens[3].type == TokenType::NUMBER);
}

TEST_CASE("Tokenizer detecta fechas en distintos formatos", "[tokenizer]") {
    auto tokens = tokenize("2024-12-31 31/12/2024 01.01.2025 2024/01/31");
    REQUIRE(tokens.size() == 4);
    for (const auto& t : tokens)
        CHECK(t.type == TokenType::DATE);
}

TEST_CASE("Tokenizer maneja signos de puntuación", "[tokenizer]") {
    auto tokens = tokenize("Hola!, cómo estás?!.");
    REQUIRE(tokens.size() == 3);
    CHECK(tokens[0].text == "hola");
    CHECK(tokens[1].text == "cómo");
    CHECK(tokens[2].text == "estás");
}

TEST_CASE("Tokenizer normaliza acentos y eñes a minúsculas", "[tokenizer]") {
    auto tokens = tokenize("ÁRBOL Ñandú CAFÉ");
    REQUIRE(tokens.size() == 3);
    CHECK(tokens[0].text == "árbol");
    CHECK(tokens[1].text == "ñandú");
    CHECK(tokens[2].text == "café");
}

TEST_CASE("Tokenizer ignora espacios múltiples y saltos de línea", "[tokenizer]") {
    auto tokens = tokenize("  uno   dos \t tres \n cuatro  ");
    REQUIRE(tokens.size() == 4);
    CHECK(tokens[0].text == "uno");
    CHECK(tokens[3].text == "cuatro");
}

TEST_CASE("Tokenizer trata correctamente palabras con guiones y apóstrofes", "[tokenizer]") {
    auto tokens = tokenize("bien-estar l'alba non-stop");
    REQUIRE(tokens.size() == 3);
    CHECK(tokens[0].text == "bien-estar");
    CHECK(tokens[1].text == "l'alba");
}

TEST_CASE("Tokenizer con cadena vacía devuelve vacío", "[tokenizer]") {
    auto tokens = tokenize("");
    CHECK(tokens.empty());
}
