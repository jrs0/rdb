#include "config.hpp"

YAML::Node load_config_file(const std::string & file_path) {
    return YAML::LoadFile(file_path);
}
