#include "tag_stats.hpp"
#include "../db/db_manager.hpp"
#include <sqlite3.h>
#include <algorithm>
#include <iostream>
#include <map>

namespace TagStats {

    constexpr float SMOOTHING_K = 0.001f;
    constexpr int NUM_TAGS = static_cast<int>(TipoPalabra::INDEFINIDO) + 1;

    void initTable() {
        sqlite3* db = Database::instance().getHandle();
        if (!db) return;

        const char* sql = R"(
            CREATE TABLE IF NOT EXISTS tag_unigrams (
                prev INTEGER NOT NULL,
                curr INTEGER NOT NULL,
                count INTEGER NOT NULL DEFAULT 0,
                PRIMARY KEY (prev, curr)
            );
            CREATE TABLE IF NOT EXISTS tag_bigrams (
                prev2 INTEGER NOT NULL,
                prev INTEGER NOT NULL,
                curr INTEGER NOT NULL,
                count INTEGER NOT NULL DEFAULT 0,
                PRIMARY KEY (prev2, prev, curr)
            );
            CREATE TABLE IF NOT EXISTS tag_trigrams (
                prev INTEGER NOT NULL,
                curr INTEGER NOT NULL,
                next INTEGER NOT NULL,
                count INTEGER NOT NULL DEFAULT 0,
                PRIMARY KEY (prev, curr, next)
            );
        )";
        char* errMsg = nullptr;
        if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
            std::cerr << "Error creating tag_stats tables: " << errMsg << std::endl;
            sqlite3_free(errMsg);
        }
    }

    void updateUnigram(TipoPalabra prev, TipoPalabra curr, int inc) {
        sqlite3* db = Database::instance().getHandle();
        if (!db) return;
        sqlite3_stmt* stmt;
        const char* sql = "INSERT INTO tag_unigrams (prev, curr, count) VALUES (?,?,?) "
                          "ON CONFLICT(prev,curr) DO UPDATE SET count = count + ?";
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, static_cast<int>(prev));
        sqlite3_bind_int(stmt, 2, static_cast<int>(curr));
        sqlite3_bind_int(stmt, 3, inc);
        sqlite3_bind_int(stmt, 4, inc);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    void updateBigram(TipoPalabra prev2, TipoPalabra prev, TipoPalabra curr, int inc) {
        sqlite3* db = Database::instance().getHandle();
        if (!db) return;
        sqlite3_stmt* stmt;
        const char* sql = "INSERT INTO tag_bigrams (prev2, prev, curr, count) VALUES (?,?,?,?) "
                          "ON CONFLICT(prev2,prev,curr) DO UPDATE SET count = count + ?";
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, static_cast<int>(prev2));
        sqlite3_bind_int(stmt, 2, static_cast<int>(prev));
        sqlite3_bind_int(stmt, 3, static_cast<int>(curr));
        sqlite3_bind_int(stmt, 4, inc);
        sqlite3_bind_int(stmt, 5, inc);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    void updateTrigram(TipoPalabra prev, TipoPalabra curr, TipoPalabra next, int inc) {
        sqlite3* db = Database::instance().getHandle();
        if (!db) return;
        sqlite3_stmt* stmt;
        const char* sql = "INSERT INTO tag_trigrams (prev, curr, next, count) VALUES (?,?,?,?) "
                          "ON CONFLICT(prev,curr,next) DO UPDATE SET count = count + ?";
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, static_cast<int>(prev));
        sqlite3_bind_int(stmt, 2, static_cast<int>(curr));
        sqlite3_bind_int(stmt, 3, static_cast<int>(next));
        sqlite3_bind_int(stmt, 4, inc);
        sqlite3_bind_int(stmt, 5, inc);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    // P(curr | prev)
    std::vector<std::pair<TipoPalabra, float>> getUnigramProbs(TipoPalabra prev) {
        sqlite3* db = Database::instance().getHandle();
        std::vector<std::pair<TipoPalabra, float>> result;
        if (!db) return result;

        sqlite3_stmt* stmt;
        const char* sql = "SELECT curr, count FROM tag_unigrams WHERE prev = ?";
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, static_cast<int>(prev));

        int total = 0;
        std::map<int, int> counts;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int curr = sqlite3_column_int(stmt, 0);
            int cnt = sqlite3_column_int(stmt, 1);
            counts[curr] = cnt;
            total += cnt;
        }
        sqlite3_finalize(stmt);

        for (int t = 0; t < NUM_TAGS; ++t) {
            int cnt = counts[t];
            float prob = (cnt + SMOOTHING_K) / (total + SMOOTHING_K * NUM_TAGS);
            if (prob > 0.0f) {
                result.emplace_back(static_cast<TipoPalabra>(t), prob);
            }
        }
        std::sort(result.begin(), result.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        return result;
    }

    // P(curr | prev2, prev)
    std::vector<std::pair<TipoPalabra, float>> getBigramProbs(TipoPalabra prev2, TipoPalabra prev) {
        sqlite3* db = Database::instance().getHandle();
        std::vector<std::pair<TipoPalabra, float>> result;
        if (!db) return result;

        sqlite3_stmt* stmt;
        const char* sql = "SELECT curr, count FROM tag_bigrams WHERE prev2 = ? AND prev = ?";
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, static_cast<int>(prev2));
        sqlite3_bind_int(stmt, 2, static_cast<int>(prev));

        int total = 0;
        std::map<int, int> counts;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int curr = sqlite3_column_int(stmt, 0);
            int cnt = sqlite3_column_int(stmt, 1);
            counts[curr] = cnt;
            total += cnt;
        }
        sqlite3_finalize(stmt);

        for (int t = 0; t < NUM_TAGS; ++t) {
            int cnt = counts[t];
            float prob = (cnt + SMOOTHING_K) / (total + SMOOTHING_K * NUM_TAGS);
            if (prob > 0.0f) {
                result.emplace_back(static_cast<TipoPalabra>(t), prob);
            }
        }
        std::sort(result.begin(), result.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        return result;
    }

    // P(curr | prev, next)   -- trigrama bidireccional
    std::vector<std::pair<TipoPalabra, float>> getTrigramProbs(TipoPalabra prev, TipoPalabra next) {
        sqlite3* db = Database::instance().getHandle();
        std::vector<std::pair<TipoPalabra, float>> result;
        if (!db) return result;

        sqlite3_stmt* stmt;
        const char* sql = "SELECT curr, count FROM tag_trigrams WHERE prev = ? AND next = ?";
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, static_cast<int>(prev));
        sqlite3_bind_int(stmt, 2, static_cast<int>(next));

        int total = 0;
        std::map<int, int> counts;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int curr = sqlite3_column_int(stmt, 0);
            int cnt = sqlite3_column_int(stmt, 1);
            counts[curr] = cnt;
            total += cnt;
        }
        sqlite3_finalize(stmt);

        for (int t = 0; t < NUM_TAGS; ++t) {
            int cnt = counts[t];
            float prob = (cnt + SMOOTHING_K) / (total + SMOOTHING_K * NUM_TAGS);
            if (prob > 0.0f) {
                result.emplace_back(static_cast<TipoPalabra>(t), prob);
            }
        }
        std::sort(result.begin(), result.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        return result;
    }

    // Carga de datos estáticos iniciales (adaptada a la nueva estructura)
    void loadDefaultFromStatic() {
        sqlite3* db = Database::instance().getHandle();
        if (!db) return;

        // Verificar si ya hay datos
        sqlite3_stmt* check;
        const char* check_sql = "SELECT COUNT(*) FROM tag_unigrams";
        sqlite3_prepare_v2(db, check_sql, -1, &check, nullptr);
        int count = 0;
        if (sqlite3_step(check) == SQLITE_ROW) count = sqlite3_column_int(check, 0);
        sqlite3_finalize(check);
        if (count > 0) return;

        updateUnigram(TipoPalabra::ART, TipoPalabra::ADJT, 80);
        updateUnigram(TipoPalabra::ART, TipoPalabra::SUST, 88);
        updateUnigram(TipoPalabra::ART, TipoPalabra::NUM, 85);
        updateUnigram(TipoPalabra::ART, TipoPalabra::PRON, 75);

        updateUnigram(TipoPalabra::PREP, TipoPalabra::SUST, 45);
        updateUnigram(TipoPalabra::PREP, TipoPalabra::PRON, 85);

        updateUnigram(TipoPalabra::SUST, TipoPalabra::VERB, 45);
        updateUnigram(TipoPalabra::SUST, TipoPalabra::ADJT, 70);

        updateUnigram(TipoPalabra::VERB, TipoPalabra::ADV, 40);
        updateUnigram(TipoPalabra::VERB, TipoPalabra::PRON, 70);

        updateUnigram(TipoPalabra::ADV, TipoPalabra::ADJT, 65);
        updateUnigram(TipoPalabra::ADV, TipoPalabra::VERB, 75);

        updateUnigram(TipoPalabra::PRON, TipoPalabra::VERB, 80);

        updateUnigram(TipoPalabra::CONJ, TipoPalabra::PRON, 90);

        updateUnigram(TipoPalabra::NUM, TipoPalabra::SUST, 85);
        updateUnigram(TipoPalabra::NUM, TipoPalabra::ADJT, 70);

        updateUnigram(TipoPalabra::ADJT, TipoPalabra::SUST, 60);

        updateUnigram(TipoPalabra::RELT, TipoPalabra::VERB, 50);

        updateUnigram(TipoPalabra::DEMS, TipoPalabra::SUST, 80);

        // Bigramas (prev2, prev, curr) – dos anteriores → actual
        updateBigram(TipoPalabra::ART, TipoPalabra::SUST, TipoPalabra::ADJT, 75);
        updateBigram(TipoPalabra::ART, TipoPalabra::SUST, TipoPalabra::NUM, 15);
        updateBigram(TipoPalabra::ART, TipoPalabra::ADJT, TipoPalabra::SUST, 88);

        updateBigram(TipoPalabra::PREP, TipoPalabra::ART, TipoPalabra::SUST, 75);
        updateBigram(TipoPalabra::PREP, TipoPalabra::SUST, TipoPalabra::VERB, 45);

        updateBigram(TipoPalabra::VERB, TipoPalabra::ADV, TipoPalabra::ADJT, 45);
        updateBigram(TipoPalabra::VERB, TipoPalabra::PRON, TipoPalabra::VERB, 70);

        updateBigram(TipoPalabra::SUST, TipoPalabra::VERB, TipoPalabra::ADV, 50);
        updateBigram(TipoPalabra::SUST, TipoPalabra::ADJT, TipoPalabra::VERB, 60);

        updateBigram(TipoPalabra::ADV, TipoPalabra::VERB, TipoPalabra::ADJT, 65);

        updateBigram(TipoPalabra::PRON, TipoPalabra::VERB, TipoPalabra::ADV, 50);

        updateBigram(TipoPalabra::CONJ, TipoPalabra::PRON, TipoPalabra::VERB, 80);

        updateBigram(TipoPalabra::NUM, TipoPalabra::SUST, TipoPalabra::ADJT, 65);

        updateBigram(TipoPalabra::PREG, TipoPalabra::VERB, TipoPalabra::ADV, 60);

        // Trigramas (prev, curr, next)
        updateTrigram(TipoPalabra::ART, TipoPalabra::SUST, TipoPalabra::ADJT, 75);
        updateTrigram(TipoPalabra::ART, TipoPalabra::SUST, TipoPalabra::NUM, 15);
        updateTrigram(TipoPalabra::ART, TipoPalabra::SUST, TipoPalabra::PRON, 5);
        updateTrigram(TipoPalabra::ART, TipoPalabra::SUST, TipoPalabra::DEMS, 2);

        updateTrigram(TipoPalabra::ART, TipoPalabra::ADJT, TipoPalabra::SUST, 88);
        updateTrigram(TipoPalabra::ART, TipoPalabra::ADJT, TipoPalabra::NUM, 6);
        updateTrigram(TipoPalabra::ART, TipoPalabra::ADJT, TipoPalabra::ADV, 3);
        updateTrigram(TipoPalabra::ART, TipoPalabra::ADJT, TipoPalabra::CONJ, 2);
        updateTrigram(TipoPalabra::ART, TipoPalabra::ADJT, TipoPalabra::PRON, 1);

        updateTrigram(TipoPalabra::ART, TipoPalabra::VERB, TipoPalabra::SUST, 80);
        updateTrigram(TipoPalabra::ART, TipoPalabra::VERB, TipoPalabra::PRON, 12);
        updateTrigram(TipoPalabra::ART, TipoPalabra::VERB, TipoPalabra::ADJT, 5);
        updateTrigram(TipoPalabra::ART, TipoPalabra::VERB, TipoPalabra::NUM, 3);

        updateTrigram(TipoPalabra::ART, TipoPalabra::PRON, TipoPalabra::SUST, 70);
        updateTrigram(TipoPalabra::ART, TipoPalabra::PRON, TipoPalabra::ADJT, 20);
        updateTrigram(TipoPalabra::ART, TipoPalabra::PRON, TipoPalabra::NUM, 10);

        updateTrigram(TipoPalabra::PREP, TipoPalabra::SUST, TipoPalabra::ART, 75);
        updateTrigram(TipoPalabra::PREP, TipoPalabra::SUST, TipoPalabra::NUM, 10);
        updateTrigram(TipoPalabra::PREP, TipoPalabra::SUST, TipoPalabra::PRON, 8);
        updateTrigram(TipoPalabra::PREP, TipoPalabra::SUST, TipoPalabra::DEMS, 5);
        updateTrigram(TipoPalabra::PREP, TipoPalabra::SUST, TipoPalabra::ADJT, 2);

        updateTrigram(TipoPalabra::PREP, TipoPalabra::VERB, TipoPalabra::ART, 50);
        updateTrigram(TipoPalabra::PREP, TipoPalabra::VERB, TipoPalabra::PRON, 25);
        updateTrigram(TipoPalabra::PREP, TipoPalabra::VERB, TipoPalabra::SUST, 15);
        updateTrigram(TipoPalabra::PREP, TipoPalabra::VERB, TipoPalabra::DEMS, 5);
        updateTrigram(TipoPalabra::PREP, TipoPalabra::VERB, TipoPalabra::NUM, 5);

        updateTrigram(TipoPalabra::PREP, TipoPalabra::PRON, TipoPalabra::VERB, 70);
        updateTrigram(TipoPalabra::PREP, TipoPalabra::PRON, TipoPalabra::PREP, 15);
        updateTrigram(TipoPalabra::PREP, TipoPalabra::PRON, TipoPalabra::ADV, 10);
        updateTrigram(TipoPalabra::PREP, TipoPalabra::PRON, TipoPalabra::CONJ, 5);

        updateTrigram(TipoPalabra::VERB, TipoPalabra::VERB, TipoPalabra::ADV, 50);
        updateTrigram(TipoPalabra::VERB, TipoPalabra::VERB, TipoPalabra::CONJ, 25);
        updateTrigram(TipoPalabra::VERB, TipoPalabra::VERB, TipoPalabra::PRON, 15);
        updateTrigram(TipoPalabra::VERB, TipoPalabra::VERB, TipoPalabra::PREP, 10);

        updateTrigram(TipoPalabra::VERB, TipoPalabra::ADJT, TipoPalabra::ADV, 55);
        updateTrigram(TipoPalabra::VERB, TipoPalabra::ADJT, TipoPalabra::VERB, 20);
        updateTrigram(TipoPalabra::VERB, TipoPalabra::ADJT, TipoPalabra::PRON, 15);
        updateTrigram(TipoPalabra::VERB, TipoPalabra::ADJT, TipoPalabra::PREP, 10);

        updateTrigram(TipoPalabra::VERB, TipoPalabra::ADV, TipoPalabra::ADJT, 45);
        updateTrigram(TipoPalabra::VERB, TipoPalabra::ADV, TipoPalabra::VERB, 25);
        updateTrigram(TipoPalabra::VERB, TipoPalabra::ADV, TipoPalabra::PREP, 15);
        updateTrigram(TipoPalabra::VERB, TipoPalabra::ADV, TipoPalabra::PRON, 10);
        updateTrigram(TipoPalabra::VERB, TipoPalabra::ADV, TipoPalabra::CONJ, 5);

        updateTrigram(TipoPalabra::VERB, TipoPalabra::SUST, TipoPalabra::ART, 50);
        updateTrigram(TipoPalabra::VERB, TipoPalabra::SUST, TipoPalabra::PREP, 25);
        updateTrigram(TipoPalabra::VERB, TipoPalabra::SUST, TipoPalabra::PRON, 15);
        updateTrigram(TipoPalabra::VERB, TipoPalabra::SUST, TipoPalabra::ADV, 10);

        updateTrigram(TipoPalabra::SUST, TipoPalabra::VERB, TipoPalabra::ADJT, 55);
        updateTrigram(TipoPalabra::SUST, TipoPalabra::VERB, TipoPalabra::PREP, 25);
        updateTrigram(TipoPalabra::SUST, TipoPalabra::VERB, TipoPalabra::PRON, 10);
        updateTrigram(TipoPalabra::SUST, TipoPalabra::VERB, TipoPalabra::CONJ, 7);
        updateTrigram(TipoPalabra::SUST, TipoPalabra::VERB, TipoPalabra::ADV, 3);

        updateTrigram(TipoPalabra::SUST, TipoPalabra::ADJT, TipoPalabra::VERB, 60);
        updateTrigram(TipoPalabra::SUST, TipoPalabra::ADJT, TipoPalabra::PREP, 30);
        updateTrigram(TipoPalabra::SUST, TipoPalabra::ADJT, TipoPalabra::CONJ, 15);
        updateTrigram(TipoPalabra::SUST, TipoPalabra::ADJT, TipoPalabra::ADV, 5);

        updateTrigram(TipoPalabra::ADV, TipoPalabra::ADV, TipoPalabra::CONJ, 45);
        updateTrigram(TipoPalabra::ADV, TipoPalabra::ADV, TipoPalabra::ADJT, 30);
        updateTrigram(TipoPalabra::ADV, TipoPalabra::ADV, TipoPalabra::VERB, 15);
        updateTrigram(TipoPalabra::ADV, TipoPalabra::ADV, TipoPalabra::PREP, 10);

        updateTrigram(TipoPalabra::ADV, TipoPalabra::VERB, TipoPalabra::ADJT, 65);
        updateTrigram(TipoPalabra::ADV, TipoPalabra::VERB, TipoPalabra::ADV, 15);
        updateTrigram(TipoPalabra::ADV, TipoPalabra::VERB, TipoPalabra::PREP, 10);
        updateTrigram(TipoPalabra::ADV, TipoPalabra::VERB, TipoPalabra::CONJ, 10);

        updateTrigram(TipoPalabra::ADV, TipoPalabra::ADJT, TipoPalabra::SUST, 60);
        updateTrigram(TipoPalabra::ADV, TipoPalabra::ADJT, TipoPalabra::VERB, 25);
        updateTrigram(TipoPalabra::ADV, TipoPalabra::ADJT, TipoPalabra::ADV, 10);
        updateTrigram(TipoPalabra::ADV, TipoPalabra::ADJT, TipoPalabra::PREP, 5);

        updateTrigram(TipoPalabra::PRON, TipoPalabra::PRON, TipoPalabra::VERB, 85);
        updateTrigram(TipoPalabra::PRON, TipoPalabra::PRON, TipoPalabra::ADV, 8);
        updateTrigram(TipoPalabra::PRON, TipoPalabra::PRON, TipoPalabra::CONJ, 5);
        updateTrigram(TipoPalabra::PRON, TipoPalabra::PRON, TipoPalabra::PREP, 2);
        updateTrigram(TipoPalabra::PRON, TipoPalabra::VERB, TipoPalabra::ADV, 50);
        updateTrigram(TipoPalabra::PRON, TipoPalabra::VERB, TipoPalabra::PREP, 25);
        updateTrigram(TipoPalabra::PRON, TipoPalabra::VERB, TipoPalabra::CONJ, 15);
        updateTrigram(TipoPalabra::PRON, TipoPalabra::VERB, TipoPalabra::PRON, 10);

        updateTrigram(TipoPalabra::CONJ, TipoPalabra::SUST, TipoPalabra::VERB, 60);
        updateTrigram(TipoPalabra::CONJ, TipoPalabra::SUST, TipoPalabra::ADJT, 20);
        updateTrigram(TipoPalabra::CONJ, TipoPalabra::SUST, TipoPalabra::PREP, 10);
        updateTrigram(TipoPalabra::CONJ, TipoPalabra::SUST, TipoPalabra::ADV, 10);

        updateTrigram(TipoPalabra::ADJT, TipoPalabra::SUST, TipoPalabra::ADV, 55);
        updateTrigram(TipoPalabra::ADJT, TipoPalabra::SUST, TipoPalabra::CUANT, 20);
        updateTrigram(TipoPalabra::ADJT, TipoPalabra::SUST, TipoPalabra::VERB, 15);
        updateTrigram(TipoPalabra::ADJT, TipoPalabra::SUST, TipoPalabra::CONJ, 10);
        updateTrigram(TipoPalabra::ADJT, TipoPalabra::VERB, TipoPalabra::ADV, 60);
        updateTrigram(TipoPalabra::ADJT, TipoPalabra::VERB, TipoPalabra::PREP, 20);
        updateTrigram(TipoPalabra::ADJT, TipoPalabra::VERB, TipoPalabra::CONJ, 15);
        updateTrigram(TipoPalabra::ADJT, TipoPalabra::VERB, TipoPalabra::PRON, 5);

        updateTrigram(TipoPalabra::NUM, TipoPalabra::SUST, TipoPalabra::ADJT, 65);
        updateTrigram(TipoPalabra::NUM, TipoPalabra::SUST, TipoPalabra::ART, 20);
        updateTrigram(TipoPalabra::NUM, TipoPalabra::SUST, TipoPalabra::NUM, 10);
        updateTrigram(TipoPalabra::NUM, TipoPalabra::SUST, TipoPalabra::PREP, 5);
        updateTrigram(TipoPalabra::NUM, TipoPalabra::ADJT, TipoPalabra::SUST, 80);
        updateTrigram(TipoPalabra::NUM, TipoPalabra::ADJT, TipoPalabra::NUM, 10);
        updateTrigram(TipoPalabra::NUM, TipoPalabra::ADJT, TipoPalabra::PREP, 10);

        updateTrigram(TipoPalabra::PREG, TipoPalabra::SUST, TipoPalabra::ADJT, 75);
        updateTrigram(TipoPalabra::PREG, TipoPalabra::SUST, TipoPalabra::NUM, 15);
        updateTrigram(TipoPalabra::PREG, TipoPalabra::SUST, TipoPalabra::VERB, 10);
        updateTrigram(TipoPalabra::PREG, TipoPalabra::VERB, TipoPalabra::ADV, 60);
        updateTrigram(TipoPalabra::PREG, TipoPalabra::VERB, TipoPalabra::SUST, 30);
        updateTrigram(TipoPalabra::PREG, TipoPalabra::VERB, TipoPalabra::ADJT, 10);
    }
}
