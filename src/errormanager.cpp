#include "errormanager.h"

ErrorManager& ErrorManager::getInstance() {
  static ErrorManager instance; //static不放在返回值上，而是在这里
  return instance;
}

ErrorManager::ErrorManager() {
  error_map_[ERRC_SUCCESS] = "Success";
  error_map_[ERRC_INVALID_MSG] = "Invalid message format";
  error_map_[ERRC_INVALID_DATA] = "Invalid data provided";    
  error_map_[ERRC_METHOD_NOT_ALLOWED] = "Method not allowed";  
  error_map_[ERRO_PROCESS_FAILED] = "Request processing failed"; 
  error_map_[ERRO_BIKE_IS_TAKEN] = "Bike has been taken by others"; 
  error_map_[ERRO_BIKE_IS_RUNNING] = "Bike is currently in use";
  error_map_[ERRO_BIKE_IS_DAMAGED] = "Bike is damaged or unavailable";
}

std::optional<std::string> ErrorManager::getDesc(i32 code) {
   auto it = error_map_.find(code);
   if (it != error_map_.end()) {
    return it->second;
   }
   return std::nullopt;
}