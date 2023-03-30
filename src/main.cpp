#include <iostream>
//#include "acs.hpp"
#include "episode.hpp"
#include "episode_row.hpp"
#include "spell_rows.hpp"
#include "spell.hpp"

int main() {    
    SpellRows rows{Timestamp{0}, 10};
    ClinicalCodeParser parser{"../../opcs4.yaml", "../../icd10.yaml"};
    Spell spell{rows, parser};
    spell.print(parser);
    
}
