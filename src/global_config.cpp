#include "global_config.h"
#include "iniparser.h"
#include <iostream>

GlobalConfig::GlobalConfig() : dictionary_(nullptr) {}
GlobalConfig::~GlobalConfig() {
  if (dictionary_ != nullptr) {
    iniparser_freedict(dictionary_);
    dictionary_ = nullptr;
  }
}

bool GlobalConfig::load(const std::string& config_path) {
  dictionary_ = iniparser_load(config_path.c_str());
  if (dictionary_ == nullptr) {
    std::cerr << "[Config] Failed to load config file: " << config_path << std::endl;
    return false;
  }
  return true;
}

std::string GlobalConfig::getString(const std::string& section, const std::string& key, const std::string& default_value) {
  if (!dictionary_) return default_value;
  
  std::string query = section + ":" + key;

  const char* result = iniparser_getstring(dictionary_, query.c_str(), default_value.c_str());

  if (result == nullptr) {
    std::cout << "[Config] Key NULL. Using default." << std::endl;
    return default_value;
  }

  return std::string(result);
}
int GlobalConfig::getInt(const std::string& section, const std::string& key, int default_value) {
  if (!dictionary_) return default_value;

  std::string query = section + ":" + key;

  return iniparser_getint(dictionary_, query.c_str(), default_value);
}