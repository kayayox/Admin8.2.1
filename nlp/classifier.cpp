#include "classifier.hpp"
#include "morphology.hpp"
#include "../utils/string_conv.hpp"
#include "../db/word_repository.hpp"
#include "../utils/classifierUtils.hpp"
#include <iostream>
#include <cctype>
#include <algorithm>

constexpr float HIGH_CONF_THRESHOLD = 0.8f;
constexpr float VALIDATION_PASS_THRESHOLD = 0.4f;
constexpr float LOW_CONF_FOR_REEVAL = 0.2f;
void testTrigramPrediction(TipoPalabra prev2,TipoPalabra prev, TipoPalabra next);
Classifier::Classifier() {}

void Classifier::updateMorphAttributes(Word& w, TipoPalabra tag) {
    const std::string& pal = w.getPalabra();
    switch (tag) {
        case TipoPalabra::VERB:
            w.setTiempo(morphology::detectTense(pal));
            w.setPersona(morphology::detectPerson(pal));
            w.setCantidad(morphology::endsWith(pal, "n") || morphology::endsWith(pal, "mos") ? Cantidad::PLUR : Cantidad::SING);
            break;
        case TipoPalabra::SUST:
        case TipoPalabra::ADJT:
            w.setCantidad(morphology::isPlural(pal) ? Cantidad::PLUR : Cantidad::SING);
            w.setGenero(morphology::detectGender(pal));
            if (tag == TipoPalabra::ADJT)
                w.setGrado(morphology::detectAdjectiveDegree(pal));
            break;
        case TipoPalabra::ART:
            w.setCantidad(morphology::isPlural(pal) ? Cantidad::PLUR : Cantidad::SING);
            break;
        default:
            break;
    }
}

void Classifier::classifySentence(std::vector<Word>& words) {
    if (words.empty()) return;

    // Paso 1: Clasificación inicial + carga desde BD
    for (auto& w : words) {
        w.load();   // carga atributos existentes

        // Si la palabra ya tiene alta confianza, se respeta y se omite el resto
        if (w.getConfianza() >= HIGH_CONF_THRESHOLD)
            continue;

        // 1a. Palabras comunes diccionario estático
        TipoPalabra commonTag;
        float commonConf;
        if (morphology::isCommonWord(w.getPalabra(), commonTag, commonConf)) {
            w.setTipo(commonTag, true);
            w.setConfianza(commonConf, true);
            updateMorphAttributes(w, commonTag);
            w.save();
            continue;   // ya esta clasificada con alta confianza
        }

        // 1b. Clasificacion morfologica inicial por sufijos
        TipoPalabra guessTag = morphology::guessInitialTag(w.getPalabra());
        float morphConf = morphology::validateTag(w.getPalabra(), guessTag);
        if (morphConf >= HIGH_CONF_THRESHOLD) {
            w.setTipo(guessTag, true);
            w.setConfianza(morphConf, true);
            updateMorphAttributes(w, guessTag);
            w.save();
        } else {
            // Etiqueta provisional baja
            w.setTipo(TipoPalabra::INDEFINIDO, true);
            w.setConfianza(LOW_CONF_FOR_REEVAL, true);
        }
    }

    // Paso 2: Refinamiento contextual iterativo
    for (size_t i = 0; i < words.size(); ++i) {
        Word& w = words[i];
        if (w.getConfianza() >= HIGH_CONF_THRESHOLD) continue; // ya es buena

        // Obtenemos contexto actualizado
        TipoPalabra prev2 = (i >= 2) ? words[i-2].getTipo() : TipoPalabra::INDEFINIDO;
        TipoPalabra prev  = (i >= 1) ? words[i-1].getTipo() : TipoPalabra::INDEFINIDO;
        TipoPalabra next  = (i+1 < words.size()) ? words[i+1].getTipo() : TipoPalabra::INDEFINIDO;

        //testTrigramPrediction(prev2,prev,next);
        TagConfidence ctx = refineTag(prev2, prev, w.getTipo(), next, w.getConfianza());
        float morphScore = morphology::validateTag(w.getPalabra(), ctx.tag);

        // Combinación de puntuaciones
        float combined = 0.6f * ctx.confidence + 0.4f * morphScore;
        w.setTipo(ctx.tag, true);
        w.setConfianza(std::min(1.0f, combined), true);
        updateMorphAttributes(w, ctx.tag);
        w.save();
    }
    // Paso 3: Actualizacion de estadisticas de transicion solo si toda la oracion es fiable
    bool sentenceHighConf = true;
    for (const auto& w : words)
        if (w.getConfianza() < HIGH_CONF_THRESHOLD) { sentenceHighConf = false; break; }

    if (sentenceHighConf) {
        for (size_t i = 1; i < words.size(); ++i)
            TagStats::updateUnigram(words[i-1].getTipo(), words[i].getTipo());
        for (size_t i = 2; i < words.size(); ++i)
            TagStats::updateBigram(words[i-2].getTipo(), words[i-1].getTipo(), words[i].getTipo());
        for (size_t i = 0; i + 2 < words.size(); ++i)
            TagStats::updateTrigram(words[i].getTipo(), words[i+1].getTipo(), words[i+2].getTipo());
    }
}

// requestCorrection se mantiene por compatibilidad, pero ya no es necesaria
void Classifier::requestCorrection(Word& w, TipoPalabra prev2, TipoPalabra prev,
                                    TipoPalabra next, float currentConfidence) {
    // Solo intentar corrección si la palabra tiene confianza baja o es indefinida
    if (currentConfidence >= HIGH_CONF_THRESHOLD && w.getTipo() != TipoPalabra::INDEFINIDO)
        return;

    // Obtener etiqueta refinada por contexto (bidireccional)
    TagConfidence refined = refineTag(prev2, prev, w.getTipo(), next, currentConfidence);
    // Combinar puntuaciones (60% contexto, 40% morfología)
    float combined = 0.6f * refined.confidence;

    if (combined >= LOW_CONF_FOR_REEVAL) {
        // La corrección es viable
        if (refined.tag != w.getTipo()) {
            // Hay cambio de etiqueta
            w.setTipo(refined.tag, true);
            w.setConfianza(std::min(1.0f, combined), true);
            updateMorphAttributes(w, refined.tag);
            std::cout << "[Corrección] '" << w.getPalabra() << "' "
                      << tipoToString(w.getTipo()) << " -> " << tipoToString(refined.tag)
                      << " (confianza " << combined * 100 << "%)\n";
        } else {
            // Misma etiqueta, solo mejorar confianza si es mayor
            if (combined > currentConfidence) {
                w.setConfianza(std::min(1.0f, combined), true);
                updateMorphAttributes(w, refined.tag); // por si acaso
            }
        }
    }
    w.save();
}

#include <iostream>
#include <iomanip>

void testTrigramPrediction(TipoPalabra prev2,TipoPalabra prev, TipoPalabra next) {
    std::cout << "\n=== TEST TRIGRAM PREDICTION ===" << std::endl;
    std::cout << "prev2 = " << static_cast<int>(prev2) << " (" << tipoToString(prev2) << ")" << std::endl;
    std::cout << "prev = " << static_cast<int>(prev) << " (" << tipoToString(prev) << ")" << std::endl;
    std::cout << "next = " << static_cast<int>(next) << " (" << tipoToString(next) << ")" << std::endl;

    // Asegurar que las tablas existen y tienen datos
    TagStats::initTable();
    TagStats::loadDefaultFromStatic(); // Solo carga si está vacío

    auto probs = TagStats::getTrigramProbs(prev, next);

    if (probs.empty()) {
        std::cout << "No predictions found (empty vector)." << std::endl;
    } else {
        /*std::cout << "Predictions (curr -> probability):" << std::endl;
        for (const auto& p : probs) {
            std::cout << "  " << std::setw(12) << tipoToString(p.first)
                      << " : " << std::fixed << std::setprecision(6) << p.second << std::endl;
        }*/
        auto best = getBestPrediction(probs);
        std::cout << "\nBest prediction: " << tipoToString(best.first)
                  << " with confidence " << best.second << std::endl;
    }
    auto probs1 = TagStats::getBigramProbs(prev2, prev);

    if (probs1.empty()) {
        std::cout << "No predictions found (empty vector)." << std::endl;
    } else {
        /*std::cout << "Predictions (curr -> probability):" << std::endl;
        for (const auto& p : probs) {
            std::cout << "  " << std::setw(12) << tipoToString(p.first)
                      << " : " << std::fixed << std::setprecision(6) << p.second << std::endl;
        }*/
        auto best1 = getBestPrediction(probs1);
        std::cout << "\nBest prediction: " << tipoToString(best1.first)
                  << " with confidence " << best1.second << std::endl;
    }
    auto probs2 = TagStats::getUnigramProbs( prev);

    if (probs2.empty()) {
        std::cout << "No predictions found (empty vector)." << std::endl;
    } else {
        /*std::cout << "Predictions (curr -> probability):" << std::endl;
        for (const auto& p : probs) {
            std::cout << "  " << std::setw(12) << tipoToString(p.first)
                      << " : " << std::fixed << std::setprecision(6) << p.second << std::endl;
        }*/
        auto best2 = getBestPrediction(probs2);
        std::cout << "\nBest prediction: " << tipoToString(best2.first)
                  << " with confidence " << best2.second << std::endl;
    }
}

void Classifier::updateConfidence(Word& w, bool acierto) {
    float conf = w.getConfianza();
    if (acierto) {
        conf += (1.0f - conf) * 0.3f;
    } else {
        conf *= 0.8f;
    }
    if (conf > 0.99f) conf = 0.99f;
    if (conf < 0.1f) conf = 0.1f;
    w.setConfianza(conf, true);
}
