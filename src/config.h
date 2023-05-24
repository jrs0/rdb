#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "yaml.h"
#include <yaml-cpp/node/parse.h>

/// Load a config file 
YAML::Node load_config_file(const std::string &file_path);

#endif
