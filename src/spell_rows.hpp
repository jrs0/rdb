#ifndef SPELL_ROWS
#define SPELL_ROWSS

#include <vector>
#include "episode_row.hpp"
#include "sql_types.hpp"
#include "random.hpp"

/*
class SpellRows {
public:
    SpellRows(const Timestamp & start, std::size_t num_episodes) {

        Seed seed{57};
        auto gen{Generator<std::size_t,0,1>(seed)};
        ClinicalCodeParser parser{"../../opcs4.yaml", "../../icd10.yaml"};

        for (std::size_t n{0}; n < num_episodes; n++) {
            EpisodeRowBuffer row{start, start + 24*60*60};
            row.set_random_fields(parser, gen);
            episode_rows_.push_back(row);
        }
    }

    /// Throws out_of_range if column does not exist, and
    /// bad_variant_access if T is not this column's type
    template<typename T>
    T at(std::string column_name) const {
        if (column_name == "spell_id") {
            if constexpr (std::is_same_v<T, Varchar>) {
return spell_id_;
            } else {
                throw std::runtime_error("spell_id is Varchar");
            }
        } else if (column_name == "spell_start") {
            if constexpr (std::is_same_v<T, Timestamp>) {
return spell_start_;
            } else {
                throw std::runtime_error("spell_start is Timestamp");
            }
        } else if (column_name == "spell_end") {
            if constexpr (std::is_same_v<T, Timestamp>) {
return spell_end_;
            } else {
                throw std::runtime_error("spell_end is Timestamp");
            }
        }
        return episode_rows_[current_row_].template at<T>(column_name);
    }

    /// Returns the number of columns
    std::size_t size() const {
        return episode_rows_[0].size();
    }

    void fetch_next_row() {
        current_row_++;
        if (current_row_ == episode_rows_.size()) {
            throw std::logic_error("No more rows");
        }
    }

private:
    std::size_t current_row_{0};
    Varchar spell_id_{"abc"};
    Timestamp spell_start_{0};
    Timestamp spell_end_{123};
    std::vector<EpisodeRowBuffer> episode_rows_;
};
*/

#endif
