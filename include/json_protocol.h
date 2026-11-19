#ifndef JSON_PROTOCOL_H
#define JSON_PROTOCOL_H

#include <stddef.h>

#define JSON_FIELD_LIMIT 32
#define JSON_KEY_LIMIT 64
#define JSON_VALUE_LIMIT 2048

typedef enum {
    JSON_STRING,
    JSON_NUMBER,
    JSON_BOOLEAN,
    JSON_NULL
} JsonValueType;

typedef struct {
    char key[JSON_KEY_LIMIT];
    char value[JSON_VALUE_LIMIT];
    JsonValueType type;
} JsonField;

typedef struct {
    JsonField fields[JSON_FIELD_LIMIT];
    size_t count;
} JsonObject;

typedef struct {
    char *data;
    size_t capacity;
    size_t length;
    int failed;
} JsonBuilder;

int json_parse_object(const char *text, JsonObject *object,
                      char *error, size_t errorSize);
const JsonField *json_get(const JsonObject *object, const char *key);
int json_get_string(const JsonObject *object, const char *key,
                    char *output, size_t outputSize, int required);
int json_get_int(const JsonObject *object, const char *key,
                 int *value, int required);
void json_builder_init(JsonBuilder *builder, char *buffer, size_t size);
void json_builder_raw(JsonBuilder *builder, const char *text);
void json_builder_format(JsonBuilder *builder, const char *format, ...);
void json_builder_string(JsonBuilder *builder, const char *text);

#endif
