#include <gtest/gtest.h>
#include "../episode.hpp"

/// Note that this test uses the codes files in the top level of
/// the repository. TODO move them somewhere accessible.
TEST(Episode, SetDiagnosesAndProcedures) {

    Episode episode;
    ClinicalCodeParser parser{"../../opcs4.yaml", "../../icd10.yaml"};

    episode.set_primary_diagnosis(parser.parse_diagnosis("I210"));
    episode.push_secondary_diagnosis(parser.parse_diagnosis("I220"));
    episode.push_secondary_diagnosis(parser.parse_diagnosis("I240"));

    episode.set_primary_procedure(parser.parse_procedure("K432"));
    episode.push_secondary_procedure(parser.parse_procedure("K111"));
    episode.push_secondary_procedure(parser.parse_procedure("K221"));

    EXPECT_EQ(episode.primary_diagnosis().name(parser), "I21.0");
    EXPECT_EQ(episode.primary_procedure().name(parser), "K43.2");
}
