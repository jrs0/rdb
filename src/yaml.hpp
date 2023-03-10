#ifndef YAML_HPP
#define YAML_HPP

#include <yaml-cpp/yaml.h>

void foo() {
    YAML::Node config = YAML::LoadFile("test.yaml");
    std::cout << config["Circles"]["x"].as<std::int64_t>();    
}

#endif
