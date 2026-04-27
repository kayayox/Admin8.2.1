/** Prototipo de Api para pruebas,es posible
que se tengan varias para propositos distintos*/
#include "nlp_api.h"
#include "core/word.hpp"
#include "dialogue/PatternCorrelator.h"
#include "core/ContextualCorrelator.h"
#include "nlp/classifier.hpp"
#include "nlp/morphology.hpp"
#include "db/db_manager.hpp"
#include "db/word_repository.hpp"
#include "db/sentence_repository.hpp"
#include "db/dialogue_repository.hpp"
#include "utils/string_conv.hpp"
#include "utils/tag_stats.hpp"
#include "tokenizer/tokenizer.hpp"
#include "core/Learning.h"
#include <sstream>
#include <iostream>
#include <memory>
#include <algorithm>
#include <cctype>

class NLPEngine::Impl {
public:
    std::string semanticDbPath;
    std::string correlatorDbPath;
    std::unique_ptr<PatternCorrelator> patternCorr;
    std::unique_ptr<ContextualCorrelator> ctxCorr;
    std::unique_ptr<Classifier> classifier;
    bool initialized = false;

    // Caché del historial de diálogos
    DialogueHistory cachedHistory;
    bool historyLoaded = false;

    bool initDB(const std::string& semPath, const std::string& corrPath) {
        semanticDbPath = semPath;
        correlatorDbPath = corrPath;
        if (!Database::instance().init(semanticDbPath)) {
            std::cerr << "[ERROR] No se pudo abrir BD semántica: " << semanticDbPath << std::endl;
            return false;
        }
        classifier = std::make_unique<Classifier>();
        try {
            patternCorr = std::make_unique<PatternCorrelator>(correlatorDbPath);
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] PatternCorrelator: " << e.what() << std::endl;
            return false;
        }
        ctxCorr = std::make_unique<ContextualCorrelator>(*patternCorr);
        initialized = true;
        return true;
    }

    void closeDB() {
        if (initialized) {
            patternCorr.reset();
            ctxCorr.reset();
            classifier.reset();
            Database::instance().close();
            initialized = false;
            historyLoaded = false;
            cachedHistory = DialogueHistory();
        }
    }

    // Carga el historial de diálogos una sola vez
    void ensureHistoryLoaded() {
        if (!historyLoaded && initialized) {
            cachedHistory = DialogueRepository::loadHistory();
            historyLoaded = true;
        }
    }

    // Convierte tokens a objetos Word (sin clasificar)
    std::vector<Word> tokensToWords(const std::vector<Token>& tokens) {
        std::vector<Word> words;
        for (const auto& tok : tokens) {
            if (tok.type == TokenType::WORD) {
                words.emplace_back(tok.text);
            } else {
                Word w(tok.text);
                w.setTipo((tok.type == TokenType::DATE) ? TipoPalabra::DATE : TipoPalabra::NUM, false);
                words.push_back(w);
            }
        }
        return words;
    }

    // Clasifica un vector de palabras usando el clasificador interno
    void classifyWordVector(std::vector<Word>& words) {
        if (!initialized || words.empty()) return;
        classifier->classifySentence(words);
    }

    void learnText(const std::string& text) {
        if (!initialized || text.empty()) {
            std::cerr << "[ERROR] learnText: motor no inicializado o texto vacío" << std::endl;
            return;
        }

        // Aprendizaje en correladores (no depende de la clasificación progresiva)
        ::learnText(*ctxCorr, *patternCorr, text);
        auto tokens = tokenize(text);
        if (tokens.empty()) return;

        std::vector<Word> words;
        for (const auto& tok : tokens) {
            if (tok.type == TokenType::WORD) {
                Word w(tok.text);
                words.push_back(w);
            } else {
                // Números o fechas: se asignan directamente, sin clasificación
                Word x(tok.text);
                x.setTipo((tok.type == TokenType::DATE) ? TipoPalabra::DATE : TipoPalabra::NUM, false);
                words.push_back(x);
            }
        }
        // Clasificación progresiva
        // se reclasifica toda la oración actual
        classifier->classifySentence(words);

        // Guardar la oración final después de toda la clasificación
        if (!words.empty()) {
            Oracion sent(words);
            sent.save();
        }
    }

    // Clasifica una oración completa y devuelve información de cada palabra
    std::vector<WordInfo> classifySentence(const std::string& sentence) {
        if (!initialized) {
            std::cerr << "[ERROR] classifySentence: motor no inicializado" << std::endl;
            return {};
        }
        auto tokens = tokenize(sentence);
        if (tokens.empty()) return {};

        std::vector<Word> words = tokensToWords(tokens);
        classifyWordVector(words);

        std::vector<WordInfo> result;
        for (const auto& w : words) {
            WordInfo info;
            info.word = w.getPalabra();
            info.tipo = tipoToString(w.getTipo());
            info.confianza = w.getConfianza();
            info.significado = w.getSignificado();
            info.cantidad = cantidadToString(w.getCantidad());
            info.tiempo = tiempoToString(w.getTiempo());
            info.genero = generoToString(w.getGenero());
            info.persona = personaToString(w.getPersona());
            info.grado = gradoToString(w.getGrado());
            result.push_back(info);
        }
        return result;
    }

    // Clasifica una palabra (usa contexto opcional)
    WordInfo classifyWord(const std::string& word, const std::string& context) {
        if (!initialized) return {};

        if (context.empty()) {
            // Sin contexto: clasificar la palabra sola
            auto tokens = tokenize(word);
            if (tokens.empty()) return {};
            std::vector<Word> words = tokensToWords(tokens);
            classifyWordVector(words);
            WordInfo info = wordToInfo(words[0]);
            return info;
        } else {
            // Con contexto: construir una pequeña oración contexto + palabra
            std::string fullSentence = context + " " + word;
            auto tokens = tokenize(fullSentence);
            std::vector<Word> words = tokensToWords(tokens);
            classifyWordVector(words);
            // Buscar la palabra objetivo (la última, suponiendo que contexto no la contiene)
            for (auto it = words.rbegin(); it != words.rend(); ++it) {
                if (it->getPalabra() == word) {
                    return wordToInfo(*it);
                }
            }
            // Fallback: si no se encuentra (por normalización), devolver la primera?
            if (!words.empty()) return wordToInfo(words.back());
            return {};
        }
    }

    // Convierte un objeto Word a WordInfo
    WordInfo wordToInfo(const Word& w) {
        WordInfo info;
        info.word = w.getPalabra();
        info.tipo = tipoToString(w.getTipo());
        info.confianza = w.getConfianza();
        info.significado = w.getSignificado();
        info.cantidad = cantidadToString(w.getCantidad());
        info.tiempo = tiempoToString(w.getTiempo());
        info.genero = generoToString(w.getGenero());
        info.persona = personaToString(w.getPersona());
        info.grado = gradoToString(w.getGrado());
        return info;
    }

    // Predice las siguientes palabras para un contexto dado
    std::vector<Prediction> predictNextWords(const std::string& currentWord,
                                             const std::vector<std::string>& previousWords) {
        if (!initialized) return {};
        std::vector<std::pair<Pattern, double>> outcomes;
        if (!ctxCorr->queryNext(currentWord, previousWords, outcomes)) {
            return {};
        }
        std::vector<Prediction> preds;
        for (const auto& p : outcomes) {
            if (!p.first.empty()) {
                Prediction pred;
                // El patrón puede contener varias palabras; tomamos la primera clave como palabra predicha
                // Nota: para patrones de múltiples palabras se podría concatenar, pero por diseño actual es una palabra.
                pred.word = p.first.begin()->first;
                pred.probability = p.second;
                preds.push_back(pred);
            }
        }
        return preds;
    }

    // Genera una hipótesis a partir de una premisa
    std::string generateHypothesis(const std::string& premise) {
        if (!initialized) return "";
        ensureHistoryLoaded(); // carga el historial una sola vez

        auto tokens = tokenize(premise);
        std::vector<Word> words = tokensToWords(tokens);
        classifyWordVector(words);
        Oracion prem(words);
        Patron pat = patronFromSecuencia(prem.getSecuenciaTipos());
        const Patron* usedPat = &pat;
        for (const auto& d : cachedHistory.getHistory()) {
            if (d.patron.tipo == pat.tipo) {
                usedPat = &d.patron;
                break;
            }
        }
        Oracion hip = ::generateHypothesis(prem, usedPat, "");
        std::stringstream ss;
        for (const auto& blk : hip.getBloques())
            ss << blk.block << " ";
        std::string result = ss.str();
        if (!result.empty() && result.back() == ' ') result.pop_back();
        return result;
    }
    void refineWordImpl(const std::string& word,
                        const std::string& prev2,
                        const std::string& prev,
                        const std::string& next) {
        if (!initialized) return;
        Word w(word);
        w.load();
        Word w2(word);
        w2.load();
        Word w1(word);
        w1.load();
        Word wn(word);
        wn.load();
        TipoPalabra t_prev2 = w2.getTipo();
        TipoPalabra t_prev  = w1.getTipo();
        TipoPalabra t_next  = wn.getTipo();
        classifier->requestCorrection(w, t_prev2, t_prev, t_next, w.getConfianza());
        // requestCorrection ya guarda en BD
    }
};

// -------------------------------------------------------------
// Implementación de la fachada pública
// -------------------------------------------------------------

NLPEngine::NLPEngine() : pImpl(std::make_unique<Impl>()) {}

NLPEngine::~NLPEngine() {
    shutdown();
}

bool NLPEngine::initialize(const std::string& semanticDbPath, const std::string& correlatorDbPath) {
    return pImpl->initDB(semanticDbPath, correlatorDbPath);
}

void NLPEngine::shutdown() {
    pImpl->closeDB();
}

void NLPEngine::learnText(const std::string& text) {
    pImpl->learnText(text);
}

std::vector<WordInfo> NLPEngine::classifySentence(const std::string& sentence) {
    return pImpl->classifySentence(sentence);
}

WordInfo NLPEngine::classifyWord(const std::string& word, const std::string& context) {
    return pImpl->classifyWord(word, context);
}

std::vector<Prediction> NLPEngine::predictNextWords(const std::string& currentWord,
                                                    const std::vector<std::string>& previousWords) {
    return pImpl->predictNextWords(currentWord, previousWords);
}

std::vector<Prediction> NLPEngine::predictNextWordWithOnePrev(const std::string& currentWord,
                                                              const std::string& previousWord) {
    return predictNextWords(currentWord, {previousWord});
}

std::vector<Prediction> NLPEngine::predictNextWordWithTwoPrev(const std::string& currentWord,
                                                              const std::string& prev1,
                                                              const std::string& prev2) {
    return predictNextWords(currentWord, {prev2, prev1});
}

std::string NLPEngine::generateHypothesis(const std::string& premiseSentence) {
    return pImpl->generateHypothesis(premiseSentence);
}

WordInfo NLPEngine::wordinfo(const Word& w){
    return pImpl->wordToInfo(w);
}

WordInfo NLPEngine::getWordInfo(const std::string& word) {
    return classifyWord(word);
}

void NLPEngine::refineWord(const std::string& word,
                               const std::string& prev2,
                               const std::string& prev,
                               const std::string& next) {
    pImpl->refineWordImpl(word, prev2, prev, next);
}

void NLPEngine::correctWord(const std::string& word, const std::string& correctType) {
    if (!pImpl->initialized) return;
    Word w(word);
    TipoPalabra newType = TipoPalabra::INDEFINIDO;
    if (correctType == "Sustantivo") newType = TipoPalabra::SUST;
    else if (correctType == "Verbo") newType = TipoPalabra::VERB;
    else if (correctType == "Adjetivo") newType = TipoPalabra::ADJT;
    else if (correctType == "Adverbio") newType = TipoPalabra::ADV;
    else if (correctType == "Preposición") newType = TipoPalabra::PREP;
    else if (correctType == "Conjunción") newType = TipoPalabra::CONJ;
    else if (correctType == "Artículo") newType = TipoPalabra::ART;
    else if (correctType == "Pronombre") newType = TipoPalabra::PRON;

    if (newType != TipoPalabra::INDEFINIDO) {
        // Guardar tipo anterior para actualizar estadísticas (solo unigrama)
        TipoPalabra oldType = w.getTipo();
        w.setTipo(newType);
        w.setConfianza(0.95f);
        w.setSignificado("Corregido por API a " + correctType);
        w.save();
    }
}
