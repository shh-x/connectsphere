#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>
#include "../include/json_protocol.h"

typedef struct {
    const char *cursor;
    char *error;
    size_t errorSize;
} JsonParser;

static void set_error(JsonParser *parser, const char *message) {
    if (parser->error && parser->errorSize)
        snprintf(parser->error, parser->errorSize, "%s", message);
}

static void skip_space(JsonParser *parser) {
    while (*parser->cursor == ' ' || *parser->cursor == '\t' ||
           *parser->cursor == '\r' || *parser->cursor == '\n')
        parser->cursor++;
}

static int append_utf8(char *output, size_t capacity, size_t *length,
                       unsigned long codepoint) {
    unsigned char bytes[4];
    size_t count, i;
    if (codepoint <= 0x7f) {
        bytes[0] = (unsigned char)codepoint;
        count = 1;
    } else if (codepoint <= 0x7ff) {
        bytes[0] = (unsigned char)(0xc0 | (codepoint >> 6));
        bytes[1] = (unsigned char)(0x80 | (codepoint & 0x3f));
        count = 2;
    } else if (codepoint <= 0xffff &&
               !(codepoint >= 0xd800 && codepoint <= 0xdfff)) {
        bytes[0] = (unsigned char)(0xe0 | (codepoint >> 12));
        bytes[1] = (unsigned char)(0x80 | ((codepoint >> 6) & 0x3f));
        bytes[2] = (unsigned char)(0x80 | (codepoint & 0x3f));
        count = 3;
    } else if (codepoint <= 0x10ffff) {
        bytes[0] = (unsigned char)(0xf0 | (codepoint >> 18));
        bytes[1] = (unsigned char)(0x80 | ((codepoint >> 12) & 0x3f));
        bytes[2] = (unsigned char)(0x80 | ((codepoint >> 6) & 0x3f));
        bytes[3] = (unsigned char)(0x80 | (codepoint & 0x3f));
        count = 4;
    } else {
        return 0;
    }
    if (*length + count >= capacity) return 0;
    for (i = 0; i < count; i++) output[(*length)++] = (char)bytes[i];
    return 1;
}

static int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int parse_hex4(JsonParser *parser, unsigned long *value) {
    int i, digit;
    unsigned long result = 0;
    for (i = 0; i < 4; i++) {
        digit = hex_value(*parser->cursor);
        if (digit < 0) return 0;
        result = result * 16 + (unsigned long)digit;
        parser->cursor++;
    }
    *value = result;
    return 1;
}

static int parse_string(JsonParser *parser, char *output, size_t outputSize) {
    size_t length = 0;
    if (*parser->cursor != '"') return 0;
    parser->cursor++;
    while (*parser->cursor && *parser->cursor != '"') {
        unsigned char c = (unsigned char)*parser->cursor++;
        if (c < 0x20) return 0;
        if (c == '\\') {
            unsigned long codepoint;
            c = (unsigned char)*parser->cursor++;
            if (c == '"' || c == '\\' || c == '/') {
                if (length + 1 >= outputSize) return 0;
                output[length++] = (char)c;
            } else if (c == 'b' || c == 'f' || c == 'n' || c == 'r' || c == 't') {
                static const char escapes[] = "bfnrt";
                static const char mapped[] = "\b\f\n\r\t";
                const char *found = strchr(escapes, (int)c);
                if (!found || length + 1 >= outputSize) return 0;
                output[length++] = mapped[found - escapes];
            } else if (c == 'u') {
                if (!parse_hex4(parser, &codepoint)) return 0;
                if (codepoint >= 0xd800 && codepoint <= 0xdbff) {
                    unsigned long low;
                    if (parser->cursor[0] != '\\' || parser->cursor[1] != 'u') return 0;
                    parser->cursor += 2;
                    if (!parse_hex4(parser, &low) || low < 0xdc00 || low > 0xdfff)
                        return 0;
                    codepoint = 0x10000 + ((codepoint - 0xd800) << 10) +
                                (low - 0xdc00);
                }
                if (!append_utf8(output, outputSize, &length, codepoint)) return 0;
            } else {
                return 0;
            }
        } else {
            if (length + 1 >= outputSize) return 0;
            output[length++] = (char)c;
        }
    }
    if (*parser->cursor != '"') return 0;
    parser->cursor++;
    output[length] = '\0';
    return 1;
}

static int parse_value(JsonParser *parser, JsonField *field) {
    const char *start;
    size_t length;
    if (*parser->cursor == '"') {
        field->type = JSON_STRING;
        return parse_string(parser, field->value, sizeof(field->value));
    }
    if (strncmp(parser->cursor, "true", 4) == 0) {
        field->type = JSON_BOOLEAN;
        strcpy(field->value, "true");
        parser->cursor += 4;
        return 1;
    }
    if (strncmp(parser->cursor, "false", 5) == 0) {
        field->type = JSON_BOOLEAN;
        strcpy(field->value, "false");
        parser->cursor += 5;
        return 1;
    }
    if (strncmp(parser->cursor, "null", 4) == 0) {
        field->type = JSON_NULL;
        strcpy(field->value, "null");
        parser->cursor += 4;
        return 1;
    }
    start = parser->cursor;
    if (*parser->cursor == '-') parser->cursor++;
    if (*parser->cursor < '0' || *parser->cursor > '9') return 0;
    if (*parser->cursor == '0') parser->cursor++;
    else while (*parser->cursor >= '0' && *parser->cursor <= '9') parser->cursor++;
    if (*parser->cursor == '.') {
        parser->cursor++;
        if (*parser->cursor < '0' || *parser->cursor > '9') return 0;
        while (*parser->cursor >= '0' && *parser->cursor <= '9') parser->cursor++;
    }
    if (*parser->cursor == 'e' || *parser->cursor == 'E') {
        parser->cursor++;
        if (*parser->cursor == '+' || *parser->cursor == '-') parser->cursor++;
        if (*parser->cursor < '0' || *parser->cursor > '9') return 0;
        while (*parser->cursor >= '0' && *parser->cursor <= '9') parser->cursor++;
    }
    length = (size_t)(parser->cursor - start);
    if (length >= sizeof(field->value)) return 0;
    memcpy(field->value, start, length);
    field->value[length] = '\0';
    field->type = JSON_NUMBER;
    return 1;
}

int json_parse_object(const char *text, JsonObject *object,
                      char *error, size_t errorSize) {
    JsonParser parser;
    if (!text || !object) return 0;
    memset(object, 0, sizeof(*object));
    parser.cursor = text;
    parser.error = error;
    parser.errorSize = errorSize;
    if (error && errorSize) error[0] = '\0';
    skip_space(&parser);
    if (*parser.cursor++ != '{') {
        set_error(&parser, "Expected a JSON object");
        return 0;
    }
    skip_space(&parser);
    if (*parser.cursor == '}') {
        parser.cursor++;
        skip_space(&parser);
        if (*parser.cursor) {
            set_error(&parser, "Unexpected data after JSON object");
            return 0;
        }
        return 1;
    }
    for (;;) {
        JsonField *field;
        size_t i;
        if (object->count >= JSON_FIELD_LIMIT) {
            set_error(&parser, "Too many JSON fields");
            return 0;
        }
        field = &object->fields[object->count];
        if (!parse_string(&parser, field->key, sizeof(field->key))) {
            set_error(&parser, "Expected a valid JSON field name");
            return 0;
        }
        for (i = 0; i < object->count; i++)
            if (strcmp(object->fields[i].key, field->key) == 0) {
                set_error(&parser, "Duplicate JSON field");
                return 0;
            }
        skip_space(&parser);
        if (*parser.cursor++ != ':') {
            set_error(&parser, "Expected ':' after JSON field name");
            return 0;
        }
        skip_space(&parser);
        if (!parse_value(&parser, field)) {
            set_error(&parser, "Expected a supported JSON scalar value");
            return 0;
        }
        object->count++;
        skip_space(&parser);
        if (*parser.cursor == '}') {
            parser.cursor++;
            break;
        }
        if (*parser.cursor++ != ',') {
            set_error(&parser, "Expected ',' or '}'");
            return 0;
        }
        skip_space(&parser);
    }
    skip_space(&parser);
    if (*parser.cursor) {
        set_error(&parser, "Unexpected data after JSON object");
        return 0;
    }
    return 1;
}

const JsonField *json_get(const JsonObject *object, const char *key) {
    size_t i;
    if (!object || !key) return NULL;
    for (i = 0; i < object->count; i++)
        if (strcmp(object->fields[i].key, key) == 0)
            return &object->fields[i];
    return NULL;
}

int json_get_string(const JsonObject *object, const char *key,
                    char *output, size_t outputSize, int required) {
    const JsonField *field = json_get(object, key);
    if (!field) {
        if (outputSize) output[0] = '\0';
        return !required;
    }
    if (field->type != JSON_STRING || strlen(field->value) >= outputSize)
        return 0;
    strcpy(output, field->value);
    return 1;
}

int json_get_int(const JsonObject *object, const char *key,
                 int *value, int required) {
    const JsonField *field = json_get(object, key);
    char *end;
    long parsed;
    if (!field) return !required;
    if (field->type != JSON_NUMBER ||
        strpbrk(field->value, ".eE") != NULL) return 0;
    parsed = strtol(field->value, &end, 10);
    if (*end || parsed < INT_MIN || parsed > INT_MAX) return 0;
    *value = (int)parsed;
    return 1;
}

void json_builder_init(JsonBuilder *builder, char *buffer, size_t size) {
    builder->data = buffer;
    builder->capacity = size;
    builder->length = 0;
    builder->failed = !buffer || size == 0;
    if (buffer && size) buffer[0] = '\0';
}

void json_builder_raw(JsonBuilder *builder, const char *text) {
    size_t length = strlen(text);
    if (builder->failed || length >= builder->capacity - builder->length) {
        builder->failed = 1;
        return;
    }
    memcpy(builder->data + builder->length, text, length + 1);
    builder->length += length;
}

void json_builder_format(JsonBuilder *builder, const char *format, ...) {
    int written;
    va_list args;
    if (builder->failed) return;
    va_start(args, format);
    written = vsnprintf(builder->data + builder->length,
                        builder->capacity - builder->length, format, args);
    va_end(args);
    if (written < 0 || (size_t)written >= builder->capacity - builder->length) {
        builder->failed = 1;
        return;
    }
    builder->length += (size_t)written;
}

void json_builder_string(JsonBuilder *builder, const char *text) {
    const unsigned char *cursor = (const unsigned char *)text;
    json_builder_raw(builder, "\"");
    while (*cursor && !builder->failed) {
        char escaped[7];
        switch (*cursor) {
        case '"': json_builder_raw(builder, "\\\""); break;
        case '\\': json_builder_raw(builder, "\\\\"); break;
        case '\b': json_builder_raw(builder, "\\b"); break;
        case '\f': json_builder_raw(builder, "\\f"); break;
        case '\n': json_builder_raw(builder, "\\n"); break;
        case '\r': json_builder_raw(builder, "\\r"); break;
        case '\t': json_builder_raw(builder, "\\t"); break;
        default:
            if (*cursor < 0x20) {
                snprintf(escaped, sizeof(escaped), "\\u%04x", (unsigned int)*cursor);
                json_builder_raw(builder, escaped);
            } else {
                char character[2] = {(char)*cursor, '\0'};
                json_builder_raw(builder, character);
            }
        }
        cursor++;
    }
    json_builder_raw(builder, "\"");
}
