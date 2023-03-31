#include <iostream>
//#include "acs.hpp"
#include "episode.hpp"
#include "episode_row.hpp"
#include "spell_rows.hpp"
#include "spell.hpp"

int main() {
    auto lookup{new_string_lookup()};
    ClinicalCodeParser parser{"../../opcs4.yaml", "../../icd10.yaml", lookup};

    // Spell spell{rows, parser};
    // spell.print(parser);
    
}
