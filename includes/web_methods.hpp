#pragma once  
#include <string>  

namespace hh_web::methods  
{  
   extern const std::string GET;  
   extern const std::string POST;  
   extern const std::string PUT;  
   extern const std::string DELETE_METHOD; // Renamed to avoid conflict with reserved keyword  
   extern const std::string PATCH;  
   extern const std::string HEAD;  
   extern const std::string OPTIONS;  
}