#include "config.h"

YAML::Node load_config_file(const std::string & file_path) {
    return YAML::LoadFile(file_path);
}
