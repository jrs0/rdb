#include <gtest/gtest.h>
#include "../episode.hpp"

/// Note that this test uses the codes files in the top level of
/// the repository. TODO move them somewhere accessible.
TEST(Episode, SetDiagnosesAndProcedures) {

    Episode episode;
    ClinicalCodeParser parser{"../../opcs4.yaml", "../../icd10.yaml"};

    // Add a selection of diagnoses
    episode.set_primary_diagnosis(parser.parse_diagnosis("I210"));
    episode.push_secondary_diagnosis(parser.parse_diagnosis("I220"));
    episode.push_secondary_diagnosis(parser.parse_diagnosis("I240"));

    // Add a selection of procedures including a duplicate
    episode.set_primary_procedure(parser.parse_procedure("K432"));
    episode.push_secondary_procedure(parser.parse_procedure("K111"));
    episode.push_secondary_procedure(parser.parse_procedure("K221"));
    episode.push_secondary_procedure(parser.parse_procedure("K221"));

    // Check the primaries
    EXPECT_EQ(episode.primary_diagnosis().name(parser), "I21.0");
    EXPECT_EQ(episode.primary_procedure().name(parser), "K43.2");

    // Check the secondaries
    auto & secondary_diagnoses{episode.secondary_diagnoses()};
    auto & secondary_procedures{episode.secondary_procedures()};

    EXPECT_EQ(secondary_diagnoses.size(), 2);
    EXPECT_EQ(secondary_procedures.size(), 3);

    EXPECT_EQ(secondary_diagnoses[0].name(parser), "I22.0");
    EXPECT_EQ(secondary_diagnoses[1].name(parser), "I24.0");

    EXPECT_EQ(secondary_procedures[0].name(parser), "K11.1");
    EXPECT_EQ(secondary_procedures[1].name(parser), "K22.1");
    EXPECT_EQ(secondary_procedures[2].name(parser), "K22.1");

}


TEST(Episode, DiagnosesAndProceduresFromRow) {
    EpisodeRowBuffer row;
    row.set_primary_diagnosis("I210");
    row.set_secondary_diagnoses({"  I220", "I240"});
    row.set_primary_procedure("K432");
    row.set_secondary_procedures({"  K111 ", "K221", "  K221 "});
    
    ClinicalCodeParser parser{"../../opcs4.yaml", "../../icd10.yaml"};
    Episode episode{row, parser};

    // Check the primaries
    EXPECT_EQ(episode.primary_diagnosis().name(parser), "I21.0");
    EXPECT_EQ(episode.primary_procedure().name(parser), "K43.2");

    // Check the secondaries
    auto & secondary_diagnoses{episode.secondary_diagnoses()};
    auto & secondary_procedures{episode.secondary_procedures()};

    EXPECT_EQ(secondary_diagnoses.size(), 2);
    EXPECT_EQ(secondary_procedures.size(), 3);

    EXPECT_EQ(secondary_diagnoses[0].name(parser), "I22.0");
    EXPECT_EQ(secondary_diagnoses[1].name(parser), "I24.0");

    EXPECT_EQ(secondary_procedures[0].name(parser), "K11.1");
    EXPECT_EQ(secondary_procedures[1].name(parser), "K22.1");
    EXPECT_EQ(secondary_procedures[2].name(parser), "K22.1");    
}
