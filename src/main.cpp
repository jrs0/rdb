#include <iostream>
//#include "acs.hpp"
#include "episode.hpp"

int main() {
    
    EpisodeRowBuffer row;
    ClinicalCodeParser parser{"../../opcs4.yaml", "../../icd10.yaml"};
    Episode episode{row, parser};
    episode.print(parser);
    
}
