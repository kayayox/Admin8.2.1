#ifndef CLASSIFIERUTILS_HPP_INCLUDED
#define CLASSIFIERUTILS_HPP_INCLUDED

#include "../types.hpp"
#include "tag_stats.hpp"
#include <vector>
#include <algorithm>

struct TagConfidence {
    TipoPalabra tag;
    float confidence;
};

inline std::vector<std::pair<TipoPalabra, float>> getUnigramPredictions(TipoPalabra prev) {
    return TagStats::getUnigramProbs(prev);
}

inline std::vector<std::pair<TipoPalabra, float>> getBigramPredictions(TipoPalabra prev2, TipoPalabra prev) {
    return TagStats::getBigramProbs(prev2, prev);
}

inline std::vector<std::pair<TipoPalabra, float>> getTrigramPredictions(TipoPalabra prev, TipoPalabra next) {
    return TagStats::getTrigramProbs(prev, next);
}

inline bool containsTag(const std::vector<std::pair<TipoPalabra, float>>& preds, TipoPalabra tag) {
    for (const auto& p : preds) if (p.first == tag) return true;
    return false;
}

inline std::pair<TipoPalabra, float> getBestPrediction(const std::vector<std::pair<TipoPalabra, float>>& preds) {
    if (preds.empty()) return {TipoPalabra::INDEFINIDO, 0.0f};
    return *std::max_element(preds.begin(), preds.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
}

TagConfidence refineTag(TipoPalabra prev2, TipoPalabra prev, TipoPalabra current,
                        TipoPalabra next, float currentConfidence) {
    // Mantener etiqueta de alta confianza
    if (current != TipoPalabra::INDEFINIDO && currentConfidence >= 0.5f) {
        return {current, currentConfidence};
    }

    // Obtener predicciones desde los tres modelos
    auto trigram = getTrigramPredictions(prev, next);   // bidireccional: (prev, next) -> curr
    auto bigram  = getBigramPredictions(prev2, prev);   // dos anteriores
    auto unigram = getUnigramPredictions(prev);         // un anterior

    // Si no hay etiqueta actual, usar la mejor predicción disponible
    if (current == TipoPalabra::INDEFINIDO) {
        if (!trigram.empty()) return {getBestPrediction(trigram).first, getBestPrediction(trigram).second};
        if (!bigram.empty())  return {getBestPrediction(bigram).first,  getBestPrediction(bigram).second};
        if (!unigram.empty()) return {getBestPrediction(unigram).first, getBestPrediction(unigram).second};
        return {current, currentConfidence};
    }

    // Si hay etiqueta actual, favorecer el trigrama bidireccional
    if (!trigram.empty()) {
        auto best = getBestPrediction(trigram);
        if (best.first == current) {
            float newConf = std::min(1.0f, currentConfidence + 0.2f);
            return {current, newConf};
        }
        return {best.first, best.second};
    }
    if (!bigram.empty()) {
        auto best = getBestPrediction(bigram);
        if (best.first == current) {
            float newConf = std::min(1.0f, currentConfidence + 0.1f);
            return {current, newConf};
        }
        return {best.first, best.second};
    }
    if (!unigram.empty()) {
        auto best = getBestPrediction(unigram);
        if (best.first == current) return {current, currentConfidence + 0.05f};
        return {best.first, best.second};
    }
    return {current, currentConfidence};
}

#endif
