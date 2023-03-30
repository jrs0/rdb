#include <iostream>
//#include "acs.hpp"
#include "episode.hpp"

int main() {
    Episode episode;

    ClinicalCodeParser parser{"../../opcs4.yaml", "../../icd10.yaml"};
    
    episode.print(parser);
}
