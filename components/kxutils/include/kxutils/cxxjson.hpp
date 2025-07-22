
#pragma once

#include "stdint.h"
#include "cJSON.h"


typedef struct cxxJSON {
    cJSON *root = NULL;

    cxxJSON() : root(cJSON_CreateObject()) { }
    cxxJSON(cJSON *root) : root(root) { }
    cxxJSON(const char *json) : root(cJSON_Parse(json)) { }

    /* bool */
    void add(const char *key, bool value) {
        if (root != NULL) {
            cJSON_AddItemToObject(root, key, cJSON_CreateBool(value));
        }
    }

    /* string */
    void add(const char *key, const char *value) {
        if (root != NULL) {
            cJSON_AddItemToObject(root, key, cJSON_CreateString(value));
        }
    }

    /* number */
    void add(const char *key, double value) {
        if (root != NULL) {
            cJSON_AddItemToObject(root, key, cJSON_CreateNumber(value));
        }
    }

    void add(const char *key, int value) {
        if (NULL != root) {
            cJSON_AddItemToObject(root, key, cJSON_CreateNumber(value));
        }
    }

    void add(const char *key, int32_t value) {
        if (NULL != root) {
            cJSON_AddItemToObject(root, key, cJSON_CreateNumber(value));
        }
    } 

    void add(const char *key, size_t value) {
        if (NULL != root) {
            cJSON_AddItemToObject(root, key, cJSON_CreateNumber(value));
        }
    }

    void add(const char *key, float value) {
        if (NULL != root) {
            cJSON_AddItemToObject(root, key, cJSON_CreateNumber(value));
        }
    }

    /* number array */
    void add(const char *key, int *value, int len) {
        if (root != NULL) {
            cJSON_AddItemToObject(root, key, cJSON_CreateIntArray(value, len));
        }
    }

    void add(const char *key, float *value, int len) {
        if (root!= NULL) {
            cJSON_AddItemToObject(root, key, cJSON_CreateFloatArray(value, len));
        }
    }

    void add(const char *key, double *value, int len) {
        if (root != NULL) {
            cJSON_AddItemToObject(root, key, cJSON_CreateDoubleArray(value, len));
        }
    }

    /* key */
    bool has(const char *key) {
        return cJSON_HasObjectItem(root, key);
    }

    bool is_exists(const char *key) {
        return cJSON_HasObjectItem(root, key);
    }

    /* 类型判断 */
    bool is_null() {
        return cJSON_IsNull(root);
    }

    bool is_object() {
        return cJSON_IsObject(root);
    }

    bool is_array() {
        return cJSON_IsArray(root);
    }

    bool is_string() {
        return cJSON_IsString(root);
    }

    bool is_number() {
        return cJSON_IsNumber(root);
    }

    /* 获取数据 */
    const char *get_string() {
        return cJSON_GetStringValue(root);
    }

    double get_double() {
        return root->valuedouble;
    }

    int get_int() {
        return root->valueint;
    }

    int array_len() {
        return cJSON_GetArraySize(root);
    }

    double array_number(int index) {
        return cJSON_GetArrayItem(root, index)->valuedouble;
    }
    
    /* operator */
    cxxJSON operator[](const char *key) {
        return cxxJSON(cJSON_GetObjectItem(root, key));
    }

    cxxJSON operator[](int index) {
        return cxxJSON(cJSON_GetArrayItem(root, index));
    }

    operator char*() const {
        return cJSON_PrintUnformatted(root);
    }

    /* 释放内存 */
    void free() {
        if (root!= NULL) {
            cJSON_Delete(root);
            root = NULL;
        }
    }

    // ~cxxJSON() {
    //     free();
    // }

}cxxJSON_Object;

// alias for backward compatibility
typedef cxxJSON_Object mJSON;