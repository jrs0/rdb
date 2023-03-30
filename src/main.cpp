#include <iostream>
//#include "acs.hpp"
#include "episode.hpp"

int main() {
    Episode episode;

    ClinicalCodeParser parser{"../../opcs4.yaml", "../../icd10.yaml"};

    episode.set_primary_diagnosis(parser.parse_diagnosis("I210"));
    episode.push_secondary_diagnosis(parser.parse_diagnosis("I220"));
    episode.push_secondary_diagnosis(parser.parse_diagnosis("I240"));

    episode.set_primary_procedure(parser.parse_procedure("K432"));
    episode.push_secondary_procedure(parser.parse_procedure("K111"));
    episode.push_secondary_procedure(parser.parse_procedure("K221"));

    episode.print(parser, 2);
}
