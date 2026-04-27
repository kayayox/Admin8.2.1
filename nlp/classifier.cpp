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
        if (morphConf >= VALIDATION_PASS_THRESHOLD) {
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
    for (int pass = 0; pass < 2; ++pass) {
        for (size_t i = 0; i < words.size(); ++i) {
            Word& w = words[i];
            if (w.getConfianza() >= HIGH_CONF_THRESHOLD) continue; // ya es buena

            // Obtenemos contexto actualizado
            TipoPalabra prev2 = (i >= 2) ? words[i-2].getTipo() : TipoPalabra::INDEFINIDO;
            TipoPalabra prev  = (i >= 1) ? words[i-1].getTipo() : TipoPalabra::INDEFINIDO;
            TipoPalabra next  = (i+1 < words.size()) ? words[i+1].getTipo() : TipoPalabra::INDEFINIDO;

            TagConfidence ctx = refineTag(prev2, prev, w.getTipo(), next, w.getConfianza());
            float morphScore = morphology::validateTag(w.getPalabra(), ctx.tag);

            // Combinación de puntuaciones
            float combined = 0.6f * ctx.confidence + 0.4f * morphScore;
            if (combined >= VALIDATION_PASS_THRESHOLD) {
                w.setTipo(ctx.tag, true);
                w.setConfianza(std::min(1.0f, combined), true);
                updateMorphAttributes(w, ctx.tag);
            } else {
                // No pasa el umbral: mantenemos INDEFINIDO pero confianza baja
                w.setTipo(TipoPalabra::INDEFINIDO, true);
                w.setConfianza(LOW_CONF_FOR_REEVAL, true);
            }
            w.save();
        }
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
    // Validar morfológicamente esa etiqueta
    float morphScore = morphology::validateTag(w.getPalabra(), refined.tag);
    // Combinar puntuaciones (60% contexto, 40% morfología)
    float combined = 0.6f * refined.confidence + 0.4f * morphScore;

    if (combined >= VALIDATION_PASS_THRESHOLD) {
        // La corrección es viable
        if (refined.tag != w.getTipo()) {
            // Hay cambio de etiqueta
            w.setTipo(refined.tag, true);
            w.setConfianza(std::min(1.0f, combined), true);
            updateMorphAttributes(w, refined.tag);
            std::cout << "[Corrección solicitada] '" << w.getPalabra() << "' "
                      << tipoToString(w.getTipo()) << " -> " << tipoToString(refined.tag)
                      << " (confianza " << combined * 100 << "%)\n";
        } else {
            // Misma etiqueta, solo mejorar confianza si es mayor
            if (combined > currentConfidence) {
                w.setConfianza(std::min(1.0f, combined), true);
                updateMorphAttributes(w, refined.tag); // por si acaso
            }
        }
    } else {
        // No pasa el umbral: degradar a indefinido con baja confianza
        if (w.getTipo() != TipoPalabra::INDEFINIDO || w.getConfianza() > LOW_CONF_FOR_REEVAL) {
            w.setTipo(TipoPalabra::INDEFINIDO, true);
            w.setConfianza(LOW_CONF_FOR_REEVAL, true);
            std::cout << "[Corrección fallida] '" << w.getPalabra() << "' pasa a INDEFINIDO (baja confianza)\n";
        }
    }
    w.save();
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
