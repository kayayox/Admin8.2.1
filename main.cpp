/** Ejemplo de uso de la Api y prueba inicial*/
#include <sstream>
#include "nlp_api.h"
#include "tokenizer/tokenizer.hpp"
#include "core/word.hpp"
#include <iostream>
#include <regex>
#include <set>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <algorithm>
#include <cctype>

// ==================== Funciones auxiliares ====================
void printWordInfo(const WordInfo& info) {
    std::cout << "Palabra: " << info.word << "\n"
              << "  Tipo: " << info.tipo << " (confianza: " << info.confianza << ")\n"
              << "  Significado: " << info.significado << "\n"
              << "  Cantidad: " << info.cantidad << ", Tiempo: " << info.tiempo
              << ", Género: " << info.genero << ", Persona: " << info.persona
              << ", Grado: " << info.grado << "\n";
}

void printPredictions(const std::vector<Prediction>& preds) {
    if (preds.empty()) {
        std::cout << "  No hay predicciones disponibles.\n";
        return;
    }
    std::cout << "  Predicciones:\n";
    for (size_t i = 0; i < preds.size(); ++i) {
        std::cout << "    " << i+1 << ". '" << preds[i].word
                  << "' (probabilidad: " << preds[i].probability << ")\n";
    }
}

bool askYesNo(const std::string& question) {
    std::string answer;
    while (true) {
        std::cout << question << " (s/n): ";
        std::getline(std::cin, answer);
        if (answer.empty()) continue;
        char first = std::tolower(answer[0]);
        if (first == 's') return true;
        if (first == 'n') return false;
        std::cout << "Respuesta no válida. Por favor responda 's' o 'n'.\n";
    }
}

// Conjunto de abreviaturas
static const std::set<std::string> ABREVIATURAS = {
    "dr", "dra", "sr", "sra", "srta", "srl", "d", "s", "v",
    "ej", "p.ej", "etc", "fig", "pág", "vol", "cap", "ed",
    "apdo", "c", "f", "j", "n", "p", "q", "rr", "ss", "vv"
};

bool esAbreviatura(const std::string& palabra) {
    return ABREVIATURAS.find(palabra) != ABREVIATURAS.end();
}

std::vector<std::string> dividirEnOraciones(const std::string& texto) {
    std::vector<std::string> oraciones;
    std::string actual;
    size_t i = 0;
    int len = texto.size();
    while (i < len) {
        actual += texto[i];
        if (texto[i] == '.' || texto[i] == '!' || texto[i] == '?') {
            size_t inicioPalabra = actual.rfind(' ', actual.size() - 2);
            if (inicioPalabra == std::string::npos) inicioPalabra = 0;
            else inicioPalabra++;
            std::string palabra = actual.substr(inicioPalabra, actual.size() - inicioPalabra - 1);
            while (!palabra.empty() && ispunct(palabra.back())) palabra.pop_back();
            std::transform(palabra.begin(), palabra.end(), palabra.begin(), ::tolower);
            bool esAbrev = esAbreviatura(palabra);
            if (!esAbrev) {
                size_t sig = i + 1;
                while (sig < len && texto[sig] == ' ') sig++;
                if (sig == len) {
                    oraciones.push_back(actual);
                    actual.clear();
                    i = sig;
                    continue;
                } else if (sig < len && isupper(texto[sig])) {
                    oraciones.push_back(actual);
                    actual.clear();
                    i = sig - 1;
                }
            }
        }
        i++;
    }
    if (!actual.empty()) {
        size_t start = actual.find_first_not_of(" \t\r\n");
        if (start != std::string::npos) {
            size_t end = actual.find_last_not_of(" \t\r\n");
            actual = actual.substr(start, end - start + 1);
            if (!actual.empty()) oraciones.push_back(actual);
        }
    }
    return oraciones;
}

bool learnFromFile(NLPEngine& engine, const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Advertencia: No se pudo abrir el archivo '" << filename << "'.\n";
        return false;
    }
    std::string contenido, linea;
    while (std::getline(file, linea)) {
        size_t start = linea.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        size_t end = linea.find_last_not_of(" \t\r\n");
        linea = linea.substr(start, end - start + 1);
        if (!contenido.empty() && !linea.empty()) contenido += ' ';
        contenido += linea;
    }
    std::vector<std::string> oraciones = dividirEnOraciones(contenido);
    int sentenceCount = 0;
    for (const auto& oracion : oraciones) {
        if (oracion.empty()) continue;
        engine.learnText(oracion);
        sentenceCount++;
        std::cout << "Aprendida oración " << sentenceCount << ": \"" << oracion << "\"\n";
    }
    std::cout << "Total de " << sentenceCount << " oraciones aprendidas desde '" << filename << "'.\n";
    return true;
}

// ==================== Menú principal ====================
void mostrarMenu() {
    std::cout << "\n=== MENÚ PRINCIPAL ===\n"
              << "1. Aprender de archivo (opcional)\n"
              << "2. Aprender una frase (texto libre)\n"
              << "3. Clasificar una oración y mostrar información\n"
              << "4. Mostrar información de una palabra\n"
              << "5. Corregir manualmente una palabra (tipo)\n"
              << "6. Refinar palabra con contexto (requestCorrection)\n"
              << "7. Entrar en modo predicción interactiva (bucle)\n"
              << "0. Salir\n"
              << "Opción: ";
}

int main() {
    // Inicializar motor
    NLPEngine engine;
    std::string semanticDb = "nlp_semantic.db";
    std::string correlatorDb = "nlp_correlations.db";
    if (!engine.initialize(semanticDb, correlatorDb)) {
        std::cerr << "Error fatal: No se pudo inicializar el motor NLP.\n";
        return 1;
    }
    std::cout << "Motor NLP inicializado correctamente.\n";

    int opcion;
    do {
        mostrarMenu();
        std::cin >> opcion;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (opcion) {
            case 1: {
                std::cout << "Nombre del archivo (ej: aprende2.txt): ";
                std::string archivo;
                std::getline(std::cin, archivo);
                if (!archivo.empty())
                    learnFromFile(engine, archivo);
                else
                    std::cout << "No se especificó archivo.\n";
                break;
            }
            case 2: {
                std::cout << "Frase para aprender: ";
                std::string frase;
                std::getline(std::cin, frase);
                if (!frase.empty()) {
                    engine.learnText(frase);
                    std::cout << "Aprendida.\n";
                }
                break;
            }
            case 3: {
                std::cout << "Oración a clasificar: ";
                std::string oracion;
                std::getline(std::cin, oracion);
                if (!oracion.empty()) {
                    auto infos = engine.classifySentence(oracion);
                    for (const auto& info : infos)
                        printWordInfo(info);
                }
                break;
            }
            case 4: {
                std::cout << "Palabra: ";
                std::string palabra;
                std::getline(std::cin, palabra);
                if (!palabra.empty()) {
                    WordInfo info = engine.getWordInfo(palabra);
                    printWordInfo(info);
                }
                break;
            }
            case 5: {
                std::cout << "Palabra a corregir: ";
                std::string palabra;
                std::getline(std::cin, palabra);
                std::cout << "Tipo correcto (Sustantivo, Verbo, Adjetivo, Adverbio, Preposición, Conjunción, Artículo, Pronombre): ";
                std::string tipo;
                std::getline(std::cin, tipo);
                if (!palabra.empty() && !tipo.empty()) {
                    engine.correctWord(palabra, tipo);
                    std::cout << "Corrección guardada.\n";
                }
                break;
            }
            case 6: {
                std::cout << "Palabra a refinar: ";
                std::string palabra;
                std::getline(std::cin, palabra);
                std::cout << "Contexto: palabra anterior2 (opcional, Enter si no): ";
                std::string p2;
                std::getline(std::cin, p2);
                std::cout << "Contexto: palabra anterior1 (opcional): ";
                std::string p1;
                std::getline(std::cin, p1);
                std::cout << "Contexto: palabra siguiente (opcional): ";
                std::string next;
                std::getline(std::cin, next);
                engine.refineWord(palabra, p2, p1, next);
                std::cout << "Refinamiento aplicado (verifique con opción 4).\n";
                break;
            }
            case 7: {
                // Modo predicción interactiva
                std::cout << "--- MODO PREDICCIÓN ---\n";
                std::cout << "Frase inicial (mínimo 2 palabras): ";
                std::string input;
                std::getline(std::cin, input);
                std::vector<std::string> words;
                std::stringstream ss(input);
                std::string w;
                while (ss >> w) words.push_back(w);
                if (words.size() < 2) {
                    std::cout << "Error: se requieren al menos 2 palabras.\n";
                    break;
                }
                std::string currentPhrase = input;
                const int MAX_ITER = 15;
                for (int iter = 1; iter <= MAX_ITER; ++iter) {
                    std::cout << "\n=== Iteración " << iter << " ===\n";
                    std::vector<std::string> tokens;
                    std::stringstream ss2(currentPhrase);
                    std::string token;
                    while (ss2 >> token) tokens.push_back(token);
                    if (tokens.size() < 2) break;
                    std::string prev1 = tokens[tokens.size()-2];
                    std::string current = tokens.back();
                    std::cout << "Contexto: anterior = '" << prev1 << "', actual = '" << current << "'\n";
                    auto preds = engine.predictNextWordWithOnePrev(current, prev1);
                    printPredictions(preds);
                    if (preds.empty()) {
                        std::cout << "No hay predicciones. Fin del bucle.\n";
                        break;
                    }
                    std::string predicted = preds[0].word;
                    double bestProb = preds[0].probability;
                    if (bestProb < 0.5 && preds.size() > 1) {
                        std::cout << "Confianza baja. Se mostrarán opciones.\n";
                        for (size_t i = 0; i < preds.size() && i < 5; ++i)
                            std::cout << "   " << i+1 << ": " << preds[i].word << " (" << preds[i].probability << ")\n";
                        std::cout << "Elija número (1-" << std::min(5, (int)preds.size()) << ") o 0 para ingresar manual: ";
                        int elec;
                        std::cin >> elec;
                        std::cin.ignore();
                        if (elec >= 1 && elec <= (int)preds.size())
                            predicted = preds[elec-1].word;
                        else if (elec == 0) {
                            std::cout << "Palabra correcta: ";
                            std::getline(std::cin, predicted);
                        }
                    } else {
                        if (!askYesNo("¿Es correcta la predicción '" + predicted + "'?")) {
                            std::cout << "Ingrese la palabra correcta: ";
                            std::getline(std::cin, predicted);
                        }
                    }
                    // Aprender la secuencia correcta
                    std::string correctedSentence = prev1 + " " + current + " " + predicted;
                    engine.learnText(correctedSentence);
                    std::cout << "Aprendida: " << correctedSentence << "\n";
                    currentPhrase += " " + predicted;
                    // Mostrar información de la palabra añadida
                    WordInfo info = engine.getWordInfo(predicted);
                    std::cout << "Información de '" << predicted << "':\n";
                    printWordInfo(info);
                    // Preguntar si se desea refinar esa palabra...no es necesario,pero sirve para la prueba
                    if (askYesNo("¿Refinar esta palabra con requestCorrection?")) {
                        engine.refineWord(predicted, (tokens.size()>=3 ? tokens[tokens.size()-3] : ""),
                                          prev1, "");
                        info = engine.getWordInfo(predicted);
                        std::cout << "Después de refinar:\n";
                        printWordInfo(info);
                    }
                }
                std::cout << "\nFrase final generada: " << currentPhrase << "\n";
                break;
            }
            case 0:
                std::cout << "Saliendo...\n";
                break;
            default:
                std::cout << "Opción no válida.\n";
        }
    } while (opcion != 0);

    engine.shutdown();
    return 0;
}
