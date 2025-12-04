#ifndef UUID_GENERATOR_H
#define UUID_GENERATOR_H

#include <random>
#include <sstream>

class UuidGenerator {
public:
  static std::string generate() {
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    static thread_local std::uniform_int_distribution<> dis1(0, 15);
    static thread_local std::uniform_int_distribution<> dis2(8, 11);

    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 8; i++) ss << dis1(gen);
    ss << "-";
    for (int i = 0; i < 4; i++) ss << dis1(gen);
    ss << "-4";
    for (int i = 0; i < 3; i++) ss << dis1(gen);
    ss << "-";
    ss << dis2(gen);
    for (int i = 0; i < 3; i++) ss << dis1(gen);
    ss << "-";
    for (int i = 0; i < 12; i++) ss << dis1(gen);

    return ss.str();
  }
};


#endif