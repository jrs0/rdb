#include <gtest/gtest.h>
#include "../episode.hpp"
#include "../episode_row.hpp"
#include "../string_lookup.hpp"
#include "../config.hpp"


/// Note that this test uses the codes files in the top level of
/// the repository. TODO move them somewhere accessible.
TEST(Episode, SetDiagnosesAndProcedures) {

    auto lookup{new_string_lookup()};
    auto config{load_config_file("../../config.yaml")};
    auto parser{new_clinical_code_parser(config["parser"], lookup)};
    Episode episode;
    
    // Add a selection of diagnoses
    episode.set_primary_diagnosis(parser->parse(CodeType::Diagnosis, "I210"));
    episode.push_secondary_diagnosis(parser->parse(CodeType::Diagnosis, "I220"));
    episode.push_secondary_diagnosis(parser->parse(CodeType::Diagnosis, "I240"));

    // Add a selection of procedures including a duplicate
    episode.set_primary_procedure(parser->parse(CodeType::Procedure, "K432"));
    episode.push_secondary_procedure(parser->parse(CodeType::Procedure, "K111"));
    episode.push_secondary_procedure(parser->parse(CodeType::Procedure, "K221"));
    episode.push_secondary_procedure(parser->parse(CodeType::Procedure, "K221"));

    // Check the primaries
    EXPECT_EQ(episode.primary_diagnosis().name(lookup), "I21.0");
    EXPECT_EQ(episode.primary_procedure().name(lookup), "K43.2");

    // Check the secondaries
    auto & secondary_diagnoses{episode.secondary_diagnoses()};
    auto & secondary_procedures{episode.secondary_procedures()};

    EXPECT_EQ(secondary_diagnoses.size(), 2);
    EXPECT_EQ(secondary_procedures.size(), 3);

    EXPECT_EQ(secondary_diagnoses[0].name(lookup), "I22.0");
    EXPECT_EQ(secondary_diagnoses[1].name(lookup), "I24.0");

    EXPECT_EQ(secondary_procedures[0].name(lookup), "K11.1");
    EXPECT_EQ(secondary_procedures[1].name(lookup), "K22.1");
    EXPECT_EQ(secondary_procedures[2].name(lookup), "K22.1");

}

/// Check that the Episodes constructor from row reads the
/// list of secondary columns correctly. In this case, all
/// contain valid codes
TEST(Episode, DiagnosesAndProceduresFromRow) {
    EpisodeRowBuffer row;
    row.set_primary_diagnosis("I210");
    row.set_secondary_diagnoses({"  I220", "I240"});
    row.set_primary_procedure("K432");
    row.set_secondary_procedures({"  K111 ", "K221", "  K221 "});

    auto lookup{new_string_lookup()};
    auto config{load_config_file("../../config.yaml")};
    auto parser{new_clinical_code_parser(config["parser"], lookup)};
    Episode episode{row, parser};

    // Check the primaries
    EXPECT_EQ(episode.primary_diagnosis().name(lookup), "I21.0");
    EXPECT_EQ(episode.primary_procedure().name(lookup), "K43.2");

    // Check the secondaries
    auto & secondary_diagnoses{episode.secondary_diagnoses()};
    auto & secondary_procedures{episode.secondary_procedures()};

    EXPECT_EQ(secondary_diagnoses.size(), 2);
    EXPECT_EQ(secondary_procedures.size(), 3);

    EXPECT_EQ(secondary_diagnoses[0].name(lookup), "I22.0");
    EXPECT_EQ(secondary_diagnoses[1].name(lookup), "I24.0");

    EXPECT_EQ(secondary_procedures[0].name(lookup), "K11.1");
    EXPECT_EQ(secondary_procedures[1].name(lookup), "K22.1");
    EXPECT_EQ(secondary_procedures[2].name(lookup), "K22.1");    
}

/// Check that the Episodes constructor short-circuits when it
/// finds an empty column
TEST(Episode, DiagnosesAndProceduresShortCircuit) {
    EpisodeRowBuffer row;
    row.set_primary_diagnosis("I210");
    row.set_secondary_diagnoses({"  I220", "I240", ""});
    row.set_primary_procedure("K432");    
    row.set_secondary_procedures({"K221", "   " , "  "});

    auto lookup{new_string_lookup()};
    auto config{load_config_file("../../config.yaml")};
    auto parser{new_clinical_code_parser(config["parser"], lookup)};
    Episode episode{row, parser};
   
    // Check the secondaries
    EXPECT_EQ(episode.secondary_diagnoses().size(), 2);
    EXPECT_EQ(episode.secondary_procedures().size(), 1);
}

/// Check that the Episodes constructor throws errors for missing
/// columns. 
TEST(Episode, EpisodeRowColumnCheck) {

    auto lookup{new_string_lookup()};
    auto config{load_config_file("../../config.yaml")};
    auto parser{new_clinical_code_parser(config["parser"], lookup)};
    
    /// Missing primary diagnosis column
    { 
	EpisodeRowBuffer row;
	row.set_primary_procedure("K432");
	EXPECT_THROW((Episode{row, parser}), std::runtime_error); 
    }

    /// Missing primary procedure column
    { 
	EpisodeRowBuffer row;
	row.set_primary_diagnosis("I210");
	EXPECT_THROW((Episode{row, parser}), std::runtime_error); 
    }
}

