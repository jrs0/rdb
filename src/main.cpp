#include <iostream>
//#include "acs.hpp"
#include "episode.hpp"

int main() {    
    EpisodeRowBuffer row;
    row.set_primary_diagnosis("I210");
    row.set_secondary_diagnoses({"  I200 ", "K231", "Z561"});
    row.set_primary_procedure("K231");
    row.set_secondary_procedures({"  K431 ", "K221", "   "});
    
    ClinicalCodeParser parser{"../../opcs4.yaml", "../../icd10.yaml"};
    Episode episode{row, parser};
    episode.print(parser);
}
