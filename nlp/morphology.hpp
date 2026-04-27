#ifndef MORPHOLOGY_HPP
#define MORPHOLOGY_HPP

#include "../types.hpp"
#include <string>
#include <vector>

namespace morphology {

    // Deteccion de propiedades gramaticales
    bool isPlural(const std::string& palabra);
    Genero detectGender(const std::string& palabra);
    Tiempo detectTense(const std::string& palabra);
    Persona detectPerson(const std::string& palabra);
    Grado detectAdjectiveDegree(const std::string& palabra);

    // Clasificacion inicial por reglas
    TipoPalabra guessInitialTag(const std::string& palabra);
    float getSuffixProb(const std::string& palabra, TipoPalabra tag);
    bool isCommonWord(const std::string& word, TipoPalabra& outTag, float& outConf);
    // NUEVA: Validar si una palabra puede tener una etiqueta dada
    float validateTag(const std::string& palabra, TipoPalabra tag);

    // Utilidades
    bool endsWith(const std::string& palabra, const std::string& sufijo);

    // Reconocimiento directo de palabras funcionales
    bool isArticle(const std::string& palabra);
    bool isPreposition(const std::string& palabra);
    bool isConjunction(const std::string& palabra);
    bool isInterrogative(const std::string& palabra);

    // Nuevas clasificaciones completadas
    bool isDemonstrative(const std::string& palabra);
    bool isNumeral(const std::string& palabra);
    bool isRelativePronoun(const std::string& palabra);
    bool isQuantifier(const std::string& palabra);
    bool isExclamation(const std::string& palabra);

} // namespace morphology

#endif // MORPHOLOGY_HPP
