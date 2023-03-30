#include <iostream>
//#include "acs.hpp"
#include "episode.hpp"

int main() {
    Episode episode;

    ClinicalCodeParser parser{"../../opcs4.yaml", "../../icd10.yaml"};

    episode.set_primary_diagnosis(parser.parse_diagnosis("I210"));
    episode.print(parser);
}
