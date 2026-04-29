# Admin8.2.1 – C++ Natural Language Processing Engine

![CI status](https://github.com/kayayox/Admin8.2.1/actions/workflows/ci.yml/badge.svg)

A modular NLP engine written in C++17 that performs morphological analysis, part‑of‑speech tagging, pattern correlation, contextual learning, and dialogue generation. It uses SQLite for persistent storage and provides a clean API for integration.

## Features

- **Part‑of‑speech tagging** with rule‑based and statistical methods (unigram/bigram/trigram models)
- **Morphological analysis** – gender, number, tense, person, degree, etc.
- **Contextual pattern learning** – learns from tri‑grams (previous two words → next word)
- **Pattern correlation** – stores word‑to‑pattern associations in a dedicated SQLite database
- **Dialogue generation** – creates hypotheses based on stored patterns and historical dialogues
- **Persistent storage** – all word properties, sentences, patterns, and dialogues are saved across sessions
- **API** – simple `NLPEngine` class for learning, classification, prediction, and correction

## Requirements

- **C++17** compiler (GCC 8+, Clang 7+, MSVC 2019+)
- **CMake** 3.14 or higher
- **SQLite3** development libraries (libsqlite3)

## Dependencies

- SQLite3 – included via `find_package(SQLite3)`

## Build Instructions

1. **Clone the repository** (or copy all source files into a directory)
   ```bash
   git clone <repository-url> Admin8.2.1
   cd Admin8.2.1
   ```

2. **Create a build directory**
   ```bash
   mkdir build && cd build
   ```

3. **Configure with CMake**
   ```bash
   cmake ..
   ```

   If SQLite3 is not in a standard location, specify its path:
   ```bash
   cmake .. -DSQLite3_ROOT=/path/to/sqlite
   ```

4. **Build**
   ```bash
   cmake --build . --config Release
   ```

   The executable `Admin8.2.1` (or `Admin8.2.1.exe` on Windows) will be created in the build directory.

## Usage

Run the interactive console application:
```bash
./Admin8.2.1
```

The program will create two SQLite databases:
- `nlp_semantic.db` – stores words, sentences, patterns, dialogues, and statistical data.
- `nlp_correlations.db` – stores pattern correlation tables for word prediction.

### Main Menu Options

1. **Learn from file** – read a text file, split it into sentences, and learn each sentence.
2. **Learn a phrase** – directly enter a sentence to learn.
3. **Classify a sentence** – display detailed tag and morphological information for each word.
4. **Show word information** – retrieve stored data for a single word.
5. **Manually correct a word** – assign a correct part‑of‑speech tag to a word.
6. **Refine a word with context** – apply contextual correction to improve an existing tag.
7. **Interactive prediction mode** – start a conversation: given two words, the engine predicts the next word, asks for confirmation, and continues.
0. **Exit**

### API Usage (for developers)

The `NLPEngine` class (defined in `nlp_api.h`) provides a high‑level interface:

```cpp
#include "nlp_api.h"

NLPEngine engine;
engine.initialize("nlp_semantic.db", "nlp_correlations.db");

// Learn from a sentence
engine.learnText("El gato duerme en la alfombra.");

// Classify a sentence
auto infos = engine.classifySentence("La casa es grande.");
for (const auto& info : infos) {
    std::cout << info.word << " : " << info.tipo << std::endl;
}

// Predict next word
std::vector<Prediction> preds = engine.predictNextWordWithOnePrev("duerme", "gato");
for (const auto& p : preds)
    std::cout << p.word << " (" << p.probability << ")" << std::endl;

// Generate a dialogue hypothesis
std::string reply = engine.generateHypothesis("¿Qué hora es?");
```

## Project Structure

```
Admin8.2.1/
├── CMakeLists.txt
├── README.md
├── main.cpp
├── nlp_api.cpp
├── nlp_api.h
├── core/                 – Word class, ContextualCorrelator, Learning helper
├── db/                   – SQLite repositories and database manager
├── dialogue/             – PatternCorrelator, pattern definitions, dialogue history
├── nlp/                  – Classifier and morphology rules
├── tokenizer/            – UTF‑8 tokenizer with number/date detection
├── utils/                – String conversions, pattern serialization, tag statistics
└── types.hpp             – Global enumerations
```

## Implementation Notes

- **UTF‑8 support**: The tokenizer handles multi‑byte characters and provides case folding for accented letters.
- **Contextual learning**: Uses up to two previous words as context to predict the next word. The `ContextualCorrelator` builds patterns from word sequences and stores correlations in a dedicated database.
- **Part‑of‑speech tagging**: Combines:
  - Static dictionaries (common words, irregular verbs, functional words)
  - Suffix‑based heuristics
  - Statistical unigram/bigram/trigram models stored in SQLite
  - Contextual refinement using `refineTag` (bidirectional trigrams)
- **Dialogue generation**: Classifies the input sentence into a pattern type (affirmation, negation, question) and applies a stored transformation to generate a hypothesis.
- **Persistent feedback**: User corrections are saved and influence future classifications.

## License

Este proyecto se proporciona "tal cual". Para licencias personalizadas, contacte al autor.

## Author

[Soubhi Khayat Najjar / kayayox@gmail.com]
Traduccion hecha con asistente IA
