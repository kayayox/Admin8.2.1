#ifndef CLASSIFIER_HPP
#define CLASSIFIER_HPP

#include "../core/word.hpp"
#include <vector>
#include <string>

class Classifier {
public:
    explicit Classifier();
    void classifySentence(std::vector<Word>& words);
    void requestCorrection(Word& w, TipoPalabra prev2, TipoPalabra prev,
                           TipoPalabra next, float currentConfidence);
    void updateConfidence(Word& w, bool acierto);
private:
    void updateMorphAttributes(Word& w, TipoPalabra tag);
};

#endif // CLASSIFIER_HPP
