#include "cJSON.h"
#include "esp_log.h"
#include <type_traits>
#include <cstddef>
#include <type_traits>
#include <cstring>
#include <utility>
#include "cJSON.h"
#include <typeinfo>
#include <iostream>

class Json
{
public:
    void JsonOverRide(cJSON *root, const char *key, double value)
    {
        // printf("double type\n");
        cJSON_AddNumberToObject(root, key, static_cast<double>(value));
    }

    void JsonOverRide(cJSON *root, const char *key, const char *value)
    {
        // printf("const char* type\n");
        cJSON_AddStringToObject(root, key, static_cast<const char *>(value));
    }

    template <typename T, typename... Args>
    void createJsonHelper(cJSON *root, T orderedPair, Args... args)
    {
        const char *key = orderedPair.first;
        auto value = orderedPair.second;
        JsonOverRide(root, key, value);
        if constexpr (sizeof...(Args) > 0)
        {
            createJsonHelper(root, args...);
        }
    }

    template <typename... Args>
    char *createJson(Args... args)
    {
        cJSON *root = cJSON_CreateObject();
        createJsonHelper(root, args...);

        char *jsonString = cJSON_PrintUnformatted(root);
        cJSON_Delete(root);

        return jsonString;
    }
};