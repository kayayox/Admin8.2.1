#ifndef TAG_STATS_HPP
#define TAG_STATS_HPP

#include "../types.hpp"
#include <vector>
#include <utility>

namespace TagStats {

    void initTable();

    // Actualización de conteos
    void updateUnigram(TipoPalabra prev, TipoPalabra curr, int inc = 1);
    void updateBigram(TipoPalabra prev2, TipoPalabra prev, TipoPalabra curr, int inc = 1);
    void updateTrigram(TipoPalabra prev, TipoPalabra curr, TipoPalabra next, int inc = 1);

    // Consultas: predicción de la etiqueta actual 'curr'
    std::vector<std::pair<TipoPalabra, float>> getUnigramProbs(TipoPalabra prev);
    std::vector<std::pair<TipoPalabra, float>> getBigramProbs(TipoPalabra prev2, TipoPalabra prev);
    std::vector<std::pair<TipoPalabra, float>> getTrigramProbs(TipoPalabra prev, TipoPalabra next);

    void loadDefaultFromStatic();
}

#endif // TAG_STATS_HPP
