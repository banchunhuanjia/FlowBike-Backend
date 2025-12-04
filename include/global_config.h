#ifndef GLOBAL_CONFIG_H
#define GLOBAL_CONFIG_H

#include <string>

// 不暴露iniparser头文件，减少编译依赖
typedef struct _dictionary_ dictionary;

class GlobalConfig {
public:
  // 单例类访问点
  static GlobalConfig& getInstance() {
    static GlobalConfig instance;
    return instance;
  }

  bool load(const std::string& config_path);

  //需要设置default
  std::string getString(const std::string& section, const std::string& key, const std::string& default_value);
  int getInt(const std::string& section, const std::string& key, int default_value);

private:
  //构造函数私有
  GlobalConfig();
  ~GlobalConfig();

  GlobalConfig(const GlobalConfig&) = delete;
  GlobalConfig& operator=(const GlobalConfig&) = delete;

  dictionary* dictionary_;
};
#endif