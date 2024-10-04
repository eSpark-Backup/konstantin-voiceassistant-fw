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

template <typename T>
void addToJsonArray(cJSON *root, const T &value) {
    // using DecayedT = typename std::decay<T>::type;

    // Handle strings
    // if (std::is_same<DecayedT, const char *>::value) {
        // Parse the string back into a cJSON object (assuming the input is a valid JSON string)
        cJSON *parsed = cJSON_Parse(value);
        if (parsed) {
            cJSON_AddItemToArray(root, parsed);
        } else {
            // If parsing fails, just add it as a string
            cJSON_AddItemToArray(root, cJSON_CreateString(value));
        }
    // }
    // // Handle integers
    // else if (std::is_integral<DecayedT>::value) {
    //     cJSON_AddItemToArray(root, cJSON_CreateNumber(static_cast<double>(value)));
    // }
    // // Handle floating point numbers
    // else if (std::is_floating_point<DecayedT>::value) {
    //     cJSON_AddItemToArray(root, cJSON_CreateNumber(value));
    // }
    // // Handle booleans
    // else if (std::is_same<DecayedT, bool>::value) {
    //     cJSON_AddItemToArray(root, cJSON_CreateBool(value));
    // }
}

    template <typename... Args>
    char *createJsonArray(Args... args) {
        cJSON *root = cJSON_CreateArray();

        // Using fold expression to process variadic template arguments
        (addToJsonArray(root, args), ...);

        // Convert JSON array to a string
        char *jsonString = cJSON_PrintUnformatted(root);

        // Clean up the cJSON object
        cJSON_Delete(root);

        return jsonString;  // Caller is responsible for freeing this memory
    }
};