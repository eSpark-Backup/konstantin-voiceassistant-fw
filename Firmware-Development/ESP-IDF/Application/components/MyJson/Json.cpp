#include"Json.hpp"
#include "cJSON.h"

// template <typename T>
// char* Json::createJson(const char* key, T* value, ...) 
// {
//     cJSON *root = cJSON_CreateObject();
    
//     cJSON_AddStringToObject(root, key, value);

//     va_list args;
//     va_start(args, value);

//     const char *nextKey = va_arg(args, const char*);
//     while (nextKey != NULL) {
//         const char *nextValue = va_arg(args, const char*);
//         cJSON_AddStringToObject(root, nextKey, nextValue);
//         nextKey = va_arg(args, const char*);
//     }

//     va_end(args);

//     char *jsonString = cJSON_PrintUnformatted(root);

//     cJSON_Delete(root);

//     return jsonString;
// }