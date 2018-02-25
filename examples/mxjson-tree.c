/*
 * ----------------------------------------------------------------------
 * |\ /| mxjson-tree.c
 * | X | Display information about a JSON file
 * |/ \| Copyright 2018 em@x35.co.uk
 * ----------------------------------------------------------------------
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "mxstr.h"
#include "mxjson.h"


/**
 * The maximum number of bytes to read from the input file at a time.
 */
#define READ_SIZE 4096


/**
 * Read input from a file descriptor into a buffer.
 *
 * @param[in] fd
 *   The file descriptor to read from.
 *
 * @param[in] buffer
 *   The buffer put the file contents into. The buffer is resized to
 *   be large enough to hold the file.
 *
 * @return
 *   Indicates whether the file was read successfully.
 */
static bool
read_file (int fd, mxbuf_t *buffer)
{
    ssize_t size;

    do {
        /*
         * Ensure there is enough space in the buffer to read the next
         * block of data (up to READ_SIZE bytes).
         */
        mxbuf_require(buffer, READ_SIZE);

        /*
         * Read the next block of data. A return value of 0 indicates the
         * end of file has been reached. A return value of -1 indicates
         * an error has occurred.
         */
        size = read(fd, buffer->available.ptr, READ_SIZE);

        if (size > 0) {
            /*
             * Mark the buffer space just filled as no longer available.
             */
            (void)mxstr_consume(&buffer->available, size);
        }
    } while (size > 0);

    /*
     * Resize the buffer to the exact size of the data read.
     */
    mxbuf_trim(buffer);

    /*
     * If size < 0 an error occurred during the call to read.
     */
    return (size == 0);
}


/**
 * Statistics about a JSON value type
 */
typedef struct {
    uint32_t count;     /**< Number of JSON values */
    size_t   size;      /**< Total size/number of children of the values */
    size_t   min_size;  /**< Minimum size across all instances */
    size_t   max_size;  /**< Maximum size across all instances */
    uint32_t named;     /**< Number of values instances with object name */
    size_t   name_size; /**< Total size of the object names */
    size_t   name_min;  /**< Minimum object name size */
    size_t   name_max;  /**< Maximum object name size */
    uint32_t name_esc;  /**< Number of object names with escape chars */
    uint32_t value_esc; /**< Number of values with escape chars */
} json_stats_t;


/**
 * Statistics for all JSON value types
 */
json_stats_t json_stats[MXJSON_COUNT];


/**
 * Get an updated minium value for a statistic
 */
static inline size_t get_min(size_t current, size_t size)
{
    return min(current == 0 ? size : current, size);
}


/**
 * Get the mean (average) value for a statistic.
 */
static inline size_t get_mean(size_t total, size_t count)
{
    size_t mean = 0;

    if (count != 0) {
        mean = (size_t)((double)total / count + 0.5);
    }

    return mean;
}


/**
 * Display the JSON statistics for a value type
 *
 * @param[in] name
 *   The name for the JSON value type.
 *
 * @param[in] stats
 *   The statistics for the JSON value type.
 *
 * @param[in,out] first
 *   Indicates whether this is the first JSON value type to be displayed.
 *   Updated to false if the statistics for the JSON value type are output.
 */
static void stats_display (const char *name, json_stats_t *stats, bool *first)
{
    if (stats->count != 0) {
        if (*first) {
            printf(" JSON   | Total    | Named    |         Name          |         Size\n");
            printf(" Type   | Count    | Count    | Min    Average   Max  |  Min   Average   Max\n");
            printf("--------+----------+----------+-------+-------+-------+-------+-------+-------\n");
            *first = false;
        }
        printf("%7s | %8u | %8u | %5lu | %5lu | %5lu | %5lu | %5lu | %5lu\n",
               name,
               stats->count, stats->named,  stats->name_min,
               get_mean(stats->name_size, stats->named), stats->name_max,
               stats->min_size, get_mean(stats->size, stats->count),
               stats->max_size);
    }
}


/**
 * Display the escape character statistics for a JSON value type
 *
 * @param[in] name
 *   The name for the JSON value type.
 *
 * @param[in] stats
 *   The statistics for the JSON value type.
 *
 * @param[in,out] first
 *   Indicates whether this is the first JSON value type to be displayed.
 *   Updated to false if the statistics for the JSON value type are output.
 */
static void escape_display (const char *name, json_stats_t *stats, bool *first)
{
    if (stats->name_esc != 0 || stats->value_esc != 0) {
        if (*first) {
            printf("\n\n");
            printf(" JSON   | Total    | Named    | Escaped  | Escaped\n");
            printf(" Type   | Count    | Count    | Name     | Value\n");
            printf("--------+----------+----------+----------+----------\n");
            *first = false;
        }

        printf("%7s | %8u | %8u | %8u | %8u\n", name,
               stats->count, stats->named,  stats->name_esc, stats->value_esc);
    }
}


/**
 * Maps JSON value types to the name for the JSON value type.
 */
static const char *json_types[MXJSON_COUNT] = {
    "None",
    "null",
    "bool",
    "number",
    "string",
    "object",
    "array"
};


/**
 * Calculate and display statistics for a JSON input.
 *
 * @param[in] p
 *   JSON parser context containing a parsed JSON input.
 */
static void
display_stats (mxjson_parser_t *p)
{
    mxjson_idx_t    idx;
    mxjson_token_t *token;
    json_stats_t   *stats;
    mxjson_type     type;
    uint32_t        total;
    uint32_t        named;
    size_t          size;
    bool            first;
    int             i;

    for (idx = 1; idx <= p->idx; idx++) {
        token = &p->tokens[idx];
        type = token->value_type;
        stats = &json_stats[type];
        stats->count++;

        if (token->name != 0) {
            stats->named++;
            size = token->name_size;
            stats->name_size += size;
            stats->name_min = get_min(stats->name_min, size);
            stats->name_max = max(stats->name_max, size);
            stats->name_esc += token->name_esc;
        }

        switch (type) {
        case MXJSON_NONE:
        case MXJSON_NULL:
            size = 0;
            break;

        case MXJSON_BOOL:
            size = 1;
            break;

        case MXJSON_NUMBER:
        case MXJSON_STRING:
            size = token->str_size;
            stats->value_esc += token->value_esc;
            break;

        case MXJSON_OBJECT:
        case MXJSON_ARRAY:
            size = token->children;
            break;

        default:
            assert(false);
            break;
        }

        stats->size += size;
        stats->min_size = get_min(stats->min_size, size);
        stats->max_size = max(stats->max_size, size);
    }

    first = true;
    total = 0;
    named = 0;
    for (i = 1; i < MXJSON_COUNT; i++) {
        stats_display(json_types[i], &json_stats[i], &first);
        total += json_stats[i].count;
        named += json_stats[i].named;
    }

    printf("--------+----------+----------+\n");
    printf(" Total: | %8u | %8u \n", total, named);

    first = true;
    for (i = 1; i < MXJSON_COUNT; i++) {
        escape_display(json_types[i], &json_stats[i], &first);
    }
}


/**
 * Calculate the depth of a token (how many nested object/array values
 * the token is inside).
 */
static inline uint16_t
token_depth (mxjson_parser_t *p, mxjson_idx_t idx)
{
    uint16_t        depth = 0;
    mxjson_token_t *token;

    assert(idx != MXJSON_IDX_NONE);

    do {
        depth++;
        token = &p->tokens[idx];
        idx = token->parent;
    } while (idx != MXJSON_IDX_NONE);

    return depth - 1;
}


/**
 * Structure to track a location within an object or array value
 */
typedef struct {
    uint32_t children; /**< Number of children in the object/array */
    uint32_t index;    /**< Current index within the object/array */
} location_t;


/**
 * Format the indentation string for a JSON value
 *
 * @param[in] loc
 *   Array of location information for ancestor object/arrays for the JSON
 *   value. The first array entry corresponds to the parent of the top-level
 *   object/array and the last array entry corresponds to the immediate parent
 *   of the JSON value.
 *
 * @param[in] depth
 *   Depth of the JSON value (number of entries in the loc array).
 *
 * @param[in] final
 *   Whether this JSON value should be displayed as the final entry in the
 *   parent object/array. This paramater is used when a subset of the
 *   object/array children is displayed and overrides the default
 *   behaviour where the last value in the object/array is displayed as the
 *   final value.
 */
static void
indent (location_t *loc, uint16_t depth, bool final)
{
    int i;

    for (i = 1; i < depth; i++) {
        if (loc[i].index == loc[i].children) {
            printf("   ");
        } else {
            printf("│  ");
        }
    }

    final = (final || loc[depth].index == loc[depth].children);

    if (depth > 0) {
        if (final) {
            printf("└─");
        } else {
            printf("├─");
        }
    }
}


/**
 * Display a hierarchical representation for a JSON value
 *
 * @param[in] p
 *   JSON parser context containing a parsed representation of the JSON value.
 *
 * @param[in] max_depth
 *   Maximum depth of the hierarchy to display.
 *
 * @param[in] array_children
 *   Maximum number of array value children to display.
 *
 * @param[in] object_children
 *   Maximum number of object value children to display.
 *
 * @param[in] annotate_size
 *   Threshold for number of children at which to add a child index annotation.
 */
static void
display_tree (mxjson_parser_t *p,
              uint16_t         max_depth,
              uint32_t         array_children,
              uint32_t         object_children,
              uint32_t         annotate_size)
{
    mxjson_idx_t    idx = 1;
    mxjson_token_t *token;
    mxjson_token_t *parent;
    int             remaining;
    uint16_t        depth = 0;
    location_t     *loc;

    loc = calloc(max_depth + 1, sizeof(*loc));
    assert(loc != NULL);

    while (idx <= p->idx) {
        token = &p->tokens[idx];
        depth = token_depth(p, idx);
        loc[depth].index++;
        indent(loc, depth, false);

        if (depth != 0 && loc[depth].children > annotate_size) {
            printf("%u/%u─", loc[depth].index, loc[depth].children);
        }

        /*
         * "*" is displayed if the name token contains escape characters.
         */
        if (token->name_esc) {
            printf("*─");
        }

        /*
         * "#" is displayed if the token value contains escape characters.
         */
        if (token->value_esc) {
            printf("#─");
        }

        printf(" ");

        if (token->name != 0) {
            printf("%.*s", token->name_size, &p->json.ptr[token->name]);
            if (token->value_type != MXJSON_OBJECT &&
                token->value_type != MXJSON_ARRAY) {
                printf(": ");
            }
        }

        idx++;
        switch (token->value_type) {
        case MXJSON_NONE:
        case MXJSON_NULL:
            printf("null");
            break;

        case MXJSON_BOOL:
            printf("%s", token->boolean ? "true" : "false");
            break;

        case MXJSON_NUMBER:
            printf("%.*s", token->str_size, &p->json.ptr[token->str]);
            break;
        case MXJSON_STRING:
            printf("\"%.*s\"", token->str_size, &p->json.ptr[token->str]);
            break;

        case MXJSON_OBJECT:
        case MXJSON_ARRAY:
            if (token->value_type == MXJSON_OBJECT) {
                printf("{%u}", token->children);
            } else {
                printf("[%u]", token->children);
            }
            break;

        default:
            assert(false);
            break;
        }

        printf("\n");

        if (token->value_type == MXJSON_OBJECT ||
            token->value_type == MXJSON_ARRAY) {

            if (depth >= max_depth) {
                if (token->children > 0) {
                    indent(loc, depth + 1, true);
                    printf("...\n");
                }
                idx = token->next;
            } else {
                loc[depth + 1].children = token->children;
                loc[depth + 1].index = 0;
            }
        }

        if (idx <= p->idx) {
            token = &p->tokens[idx];
            parent = &p->tokens[token->parent];
            depth = token_depth(p, idx);

            /*
             * Handle the completion of the display for object/array
             * values.
             */
            while (idx <= p->idx &&
                   ((parent->value_type == MXJSON_ARRAY &&
                     loc[depth].index >= array_children) ||
                    (parent->value_type == MXJSON_OBJECT &&
                     loc[depth].index >= object_children))) {

                remaining = loc[depth].children - loc[depth].index;

                if (remaining > 0) {
                    indent(loc, depth, true);
                    printf("... (%u more, %u total)\n", remaining,
                        loc[depth].children);
                }

                idx = parent->next;
                token = &p->tokens[idx];
                parent = &p->tokens[token->parent];
                depth = token_depth(p, idx);
            }
        }

    }

    free(loc);
}


int main (int argc, char **argv)
{
    int             opt;
    const char     *status;
    mxbuf_t         data;
    mxstr_t         json;
    size_t          parsed_len;
    mxjson_parser_t p;
    uint32_t        max_array_size = 20;
    uint32_t        max_object_size = 100;
    uint32_t        annotate_size = 20;
    uint16_t        max_depth = 100;
    const char     *filename;
    int             fd;
    bool            stats = false;
    bool            tree = false;
    bool            ok = true;

    while ((opt = getopt(argc, argv, "a:d:hi:o:st")) != -1) {
        switch (opt) {
        case 'a':
            tree = true;
            max_array_size = atoi(optarg);
            break;

        case 'd':
            tree = true;
            max_depth = atoi(optarg);
            break;

        case 'i':
            tree = true;
            annotate_size = atoi(optarg);
            break;

        case 'o':
            tree = true;
            max_object_size = atoi(optarg);
            break;

        case 's':
            stats = true;
            break;

        case 't':
            tree = true;
            break;

        case 'h':
        default:
            fprintf(stderr, "Usage: %s [OPTION...] [FILE]\n\n"
             "  -a <count>  Maximum number of array entries to display\n"
             "  -d <depth>  Maximum depth to display\n"
             "  -h          Display this usage information\n"
             "  -i <count>  Threshold at which to annotate with index values\n"
             "  -o <count>  Maximum number of object children to display\n"
             "  -s          Display statistics for each JSON token type\n"
             "  -t          Display JSON hierarchy\n\n"
             "If FILE is not specified, input is read from stdin.\n\n"
             "The JSON hierarchy is displayed if any of -a, -d, -o, -s or -t\n"
             "are specfiied.\n\n", argv[0]);
            fprintf(stderr, "Default values: %s -a %u -d %u -i %u -o %u\n\n",
                    argv[0],
                    max_array_size,
                    max_depth,
                    annotate_size,
                    max_object_size);
            exit(1);
            break;
        }
    }

    if (optind >= argc) {
        fd = STDIN_FILENO;
    } else {
        filename = argv[optind];
        fd = open(filename, O_RDONLY);
        ok = (fd != -1);
    }

    mxjson_init(&p, 1024, NULL, mxjson_resize);
    mxbuf_create(&data, NULL, 0);

    if (ok) {
        ok = read_file(fd, &data);
    }

    if (!ok) {
        printf("Could not read file\n");
    } else {
        /*
         * Parse the JSON input
         */
        json = mxbuf_str(&data);
        ok = mxjson_parse(&p, json);

        if (p.idx >= p.count) {
            assert(!ok);
            status = "Insufficient token memory";

        } else if (!mxstr_empty(p.unparsed)) {
            assert(!ok);
            status = "Invalid JSON";

        } else {
            assert(ok);
            status = "Valid JSON";
        }

        parsed_len = p.json.len - p.unparsed.len;
        printf("Parsed: %lu / %lu bytes (%s)\n", parsed_len, json.len, status);

        if (stats) {
            printf("\n");
            display_stats(&p);
            printf("\n");
        }

        if (tree) {
            printf("\n");
            display_tree(&p,
                         max_depth,
                         max_array_size,
                         max_object_size,
                         annotate_size);
            printf("\n");
        }
    }

    mxjson_free(&p);
    mxbuf_free(&data);

    return (!ok);
}
