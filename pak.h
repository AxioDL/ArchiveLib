#ifndef PAK_H
#define PAK_H

#include <stdint.h>
#include <stdbool.h>
#include <memory.h>
#include <malloc.h>
#include <endian.h>

#define MAKEFOURCC(a, b, c, d) (((uint32_t)a) | (((uint32_t)b) << 8) | (((uint32_t)c) << 16) | (((uint32_t)d) << 24))

#define PAK_VERSION_MAJOR 0
#define PAK_VERSION_MINOR 1
#define PAK_VERSION_PATCH 0
#define PAK_VERSION MAKEFOURCC(PAK_VERSION_MAJOR, PAK_VERSION_MINOR, PAK_VERSION_PATCH, 0)
#define PAK_MAGIC MAKEFOURCC('P', 'A', 'K', '0' + PAK_VERSION_MAJOR)

#define PAK_ENTRY_FLAGS_DIR        (1 << 0)
#define PAK_ENTRY_FLAGS_COMPRESSED (1 << 1)
#define PAK_ENTRY_FLAGS_WRITEABLE  (1 << 2)

#define PAK_ENTRY_IS_DIR(ent) (((ent)->flags & PAK_ENTRY_FLAGS_DIR) == PAK_ENTRY_FLAGS_DIR)

#define pak_alloc(size) malloc(size)
#define pak_clear(buf, size) memset((void*)buf, 0xFF, size)
#define pak_free(buf) free((void*)buf)

typedef enum { BigEndian, LittleEndian } pak_endian;

typedef struct _pak_header {
    uint32_t  magic;
    uint32_t  version;
    uint16_t  endian;
    uint64_t  entry_start;
    uint64_t  entry_count;
    uint64_t  string_table_offset;
    uint64_t  string_table_size;
    uint64_t  data_offset;
} __attribute__((packed)) pak_header_t;

typedef struct _pak_entry {
    uint8_t  flags;
    uint64_t file_id;                   // Simply an index of the file.
    int64_t string_offset;             // relative to string_table_offset specified in the header
    int64_t data_offset_or_first_child;// relative to data_offset specified in the header unless it's a directory, then it's the index to the first entry
    int64_t data_size_or_child_count;  // if directory child count, otherwise datasize
    int64_t data_uncompressed_size;    // if flags has it's compressed bit set, check this value, otherwise assume it's uncompressed
} __attribute__((packed)) pak_entry_t;

typedef struct _pak_node pak_node_t;

struct _pak_node {
    pak_node_t* next;
    pak_node_t* prev;
    pak_node_t* child;
    pak_node_t* parent;

    pak_entry_t* entry;

    char filename[FILENAME_MAX];
    bool is_dir;
};

typedef struct _pak_handle {
    FILE* file;
    const char* filename;
    pak_header_t* header;
    void* entry_table_data;
    void* string_table_data;
    // set internally
    const bool    is_readonly;

    pak_node_t* root;
} pak_handle_t;

typedef struct _pak_file {
    pak_handle_t* handle;
    pak_node_t* node;
    int64_t position;
    const char filepath[FILENAME_MAX];
} pak_file_t;

#ifdef __cplusplus
extern "C" {
#endif

pak_handle_t* pak_open_read(const char* filename);

pak_handle_t*  pak_open_write(const char* filename);

size_t pak_seek(pak_handle_t* handle, uint64_t offset, int whence);

void pak_close(pak_handle_t* handle);

pak_header_t* pak_create_header();
void pak_free_header(pak_header_t* header);

uint32_t pak_get_version(pak_handle_t* handle);

void pak_set_endian(pak_handle_t* handle, int endian);
int32_t pak_get_endian(pak_handle_t* handle);

void pak_set_entry_start(pak_handle_t* handle, uint64_t val);
uint64_t pak_get_entry_start(pak_handle_t* handle);

void pak_set_entry_count(pak_handle_t* handle, uint64_t val);
uint64_t pak_get_entry_count(pak_handle_t* handle);

void pak_set_string_table_offset(pak_handle_t* handle, uint64_t val);
uint64_t pak_get_string_table_offset(pak_handle_t* handle);

void pak_set_string_table_size(pak_handle_t* handle, uint64_t val);
uint64_t pak_get_string_table_size(pak_handle_t* header);

void pak_set_data_offset(pak_handle_t* handle, uint64_t val);
uint64_t pak_get_data_offset(pak_handle_t* handle);

int64_t pak_get_index_from_entry(pak_handle_t* handle, pak_entry_t* entry);

pak_node_t* pak_find_file(pak_handle_t* handle, const char* filepath);
pak_node_t* pak_find_dir(pak_handle_t* handle, const char* path);
pak_node_t* pak_find(pak_handle_t* handle, const char* filepath);

pak_file_t* pak_open_file(pak_handle_t*, const char* filepath);
void pak_close_file(pak_file_t* handle);

uint32_t pak_file_read_uint(pak_file_t* file);

size_t pak_file_seek(pak_file_t* file, int64_t offset, int whence);

pak_entry_t* pak_create_entry();
void pak_free_entry(pak_entry_t* entry);

pak_node_t* pak_create_node();
void pak_free_node(pak_node_t*);

#ifdef __cplusplus
}
#endif

#endif // PAK_H

