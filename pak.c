#include "pak.h"
#include <endian.h>
#include <assert.h>

#if __BYTE_ORDER__ == __BIG_ENDIAN
#define PAK_ENDIAN_BIG    0xFEFF
#define PAK_ENDIAN_LITTLE 0xFFFE
#elif __BYTE_ORDER__ == __LITTLE_ENDIAN
#define PAK_ENDIAN_BIG    0xFFFE
#define PAK_ENDIAN_LITTLE 0xFEFF
#endif

//static pak_node_t* current_dir = NULL;

static void build_node_tree(pak_handle_t* handle);

pak_handle_t* pak_open_read(const char* filename) {
    pak_handle_t* handle = pak_alloc(sizeof(pak_handle_t));
    assert(handle);
    memset(handle, 1, sizeof(pak_handle_t));
    handle->file = fopen(filename, "rb");
    handle->filename = filename;
    handle->header = pak_create_header();
    handle->root = pak_create_node();
    handle->root->entry = pak_create_entry();
    handle->root->entry->flags = PAK_ENTRY_FLAGS_DIR | PAK_ENTRY_FLAGS_WRITEABLE;
    handle->root->entry->data_offset_or_first_child = 0;

    handle->root->is_dir = true;
    // we wan't an empty filename, this way it always matches if the user request "/"
    strcat(handle->root->filename, "\0");

    if (handle->file) {
        if (fread(handle->header, 1, sizeof(pak_header_t), handle->file) != sizeof(pak_header_t))
            goto fail;

        if (handle->header->magic != PAK_MAGIC || handle->header->version != PAK_VERSION)
            goto fail;

        uint32_t entry_table_size = handle->header->entry_count * sizeof(pak_entry_t);
        handle->entry_table_data = malloc(entry_table_size);
        fseek(handle->file, handle->header->entry_start, SEEK_SET);
        if (fread(handle->entry_table_data, 1, entry_table_size, handle->file) != entry_table_size) {
            free(handle->entry_table_data);
            goto fail;
        }

        handle->string_table_data = malloc(handle->header->string_table_size);
        fseek(handle->file, handle->header->string_table_offset, SEEK_SET);
        if (fread(handle->string_table_data, 1, handle->header->string_table_size, handle->file) != handle->header->string_table_size) {
            free(handle->string_table_data);
            goto fail;
        }
        handle->root->entry->data_size_or_child_count = handle->header->entry_count;

        build_node_tree(handle);

        return handle;
    }

fail:
    pak_close(handle);
    return NULL;
}

pak_handle_t* pak_open_write(const char* filename) {
    pak_handle_t* handle = pak_alloc(sizeof(pak_handle_t));
    assert(handle);
    memset(handle, 0, sizeof(pak_handle_t));
    handle->file = fopen(filename, "r+b");
    handle->filename = filename;
    handle->root = NULL;

    if (handle->file)
    {
        handle->header = pak_create_header();
        return handle;
    }
    return NULL;
}

void pak_close(pak_handle_t* handle) {
    assert(handle);
    fseek(handle->file, 0, SEEK_SET);
    fwrite(handle->header, 1, sizeof(pak_header_t), handle->file);
    free(handle->entry_table_data);
    free(handle->string_table_data);
    if (handle->root)
        pak_free_node(handle->root);
    fclose(handle->file);
    pak_free(handle);
}

size_t pak_seek(pak_handle_t* handle, uint64_t offset, int whence) {
    assert(handle);
    return fseek(handle->file, offset, whence);
}

pak_header_t* pak_create_header() {
    pak_header_t* ret = NULL;
    ret = pak_alloc(sizeof(pak_header_t));
    assert(ret);
    pak_clear(ret, sizeof(pak_header_t));

    ret->magic = PAK_MAGIC;
    ret->version = PAK_VERSION;
    ret->endian = 0xFEFF;

    return ret;
}

void pak_free_header(pak_header_t* header) {
    assert(header);
    pak_free(header);
}

uint32_t pak_get_version(pak_handle_t* handle) {
    assert(handle);
    assert(handle->header);
    return handle->header->version;
}


void pak_set_endian(pak_handle_t* handle, int endian) {
    assert(handle);
    assert(handle->header);
    switch(endian) {
        case BigEndian:
            handle->header->endian = PAK_ENDIAN_BIG;
            break;
        case LittleEndian:
            handle->header->endian = PAK_ENDIAN_LITTLE;
            break;
    }
}


int32_t pak_get_endian(pak_handle_t* handle) {
    assert(handle);
    assert(handle->header);
    switch(handle->header->endian) {
        case PAK_ENDIAN_BIG:
            return BigEndian;
        case PAK_ENDIAN_LITTLE:
            return BigEndian;
    }

    return -1;
}

void pak_set_string_table_offset(pak_handle_t* handle, uint64_t val) {
    assert(handle);
    assert(handle->header);
    assert(val > 0);
    handle->header->string_table_offset = val;
}

uint64_t pak_get_string_table_offset(pak_handle_t* handle) {
    assert(handle);
    assert(handle->header);
    return handle->header->string_table_offset;
}

void pak_set_string_table_size(pak_handle_t* handle, uint64_t val) {
    assert(handle);
    assert(handle->header);
    assert(val > 0);
    handle->header->string_table_size = val;
}

uint64_t pak_get_string_table_size(pak_handle_t* handle) {
    assert(handle);
    assert(handle->header);
    return handle->header->string_table_size;
}

void pak_set_entry_start(pak_handle_t* handle, uint64_t val) {
    assert(handle);
    assert(handle->header);
    assert(val > 0);
    handle->header->entry_start = val;
}

uint64_t pak_get_entry_start(pak_handle_t* handle) {
    assert(handle);
    assert(handle->header);
    return handle->header->entry_start;
}

void pak_set_entry_count(pak_handle_t* handle, uint64_t val) {
    assert(handle);
    assert(handle->header);
    assert(val > 0);

    handle->header->entry_count = val;
}

uint64_t pak_get_entry_count(pak_handle_t* handle) {
    assert(handle);
    assert(handle->header);
    return handle->header->entry_count;
}

void pak_set_data_offset(pak_handle_t* handle, uint64_t val) {
    assert(handle);
    assert(handle->header);
    assert(val > 0);
    handle->header->data_offset = val;
}

uint64_t pak_get_data_offset(pak_handle_t* handle) {
    assert(handle);
    assert(handle->header);
    return handle->header->data_offset;
}

pak_entry_t* pak_get_entry_from_index(pak_handle_t* handle, uint64_t index) {
    assert(handle);
    assert(handle->header);
    return (handle->entry_table_data + (sizeof(pak_entry_t) * index));
}

const char* pak_get_string_from_index(pak_handle_t* handle, uint64_t index) {
    assert(handle);
    assert(handle->header);
    pak_entry_t* entry = pak_get_entry_from_index(handle, index);
    return (const char*)(handle->string_table_data + entry->string_offset);
}

int32_t pak_get_index_from_entry(pak_handle_t* handle, pak_entry_t* entry) {
    assert(handle);
    assert(handle->header);
    uint64_t index = 0;
    while (index < handle->header->entry_count) {
        uint32_t offset = sizeof(pak_entry_t) * index;
        if (!memcmp(handle->entry_table_data + offset, entry, sizeof(pak_entry_t))) {
            return index;
        }
    }

    return -1;
}

pak_entry_t* pak_create_entry() {
    pak_entry_t* ret = NULL;
    ret = pak_alloc(sizeof(pak_entry_t));
    assert(ret);
    pak_clear(ret, sizeof(pak_entry_t));

    return ret;
}

void pak_free_entry(pak_entry_t* entry) {
    assert(entry);
    pak_free(entry);
}


static uint64_t current_idx = 0;
static pak_node_t* current_node = NULL;

void build_node_tree_recursive(pak_handle_t* handle, pak_node_t* parent) {
    assert(handle);
    pak_node_t* tmp = pak_create_node();
    tmp->entry = pak_get_entry_from_index(handle, current_idx);
    strcpy(tmp->filename, pak_get_string_from_index(handle, current_idx));
    tmp->is_dir = PAK_ENTRY_IS_DIR(tmp->entry);
    current_idx++;

    if (parent && (!parent->child)) {
        parent->child = tmp;
        tmp->prev = parent;
    } else {
        current_node->next = tmp;
        tmp->prev = current_node;
    }

    if (parent) {
        tmp->parent = parent;
    }

    if (tmp->is_dir) {
        uint32_t tmpIdx = 0;
        while ((tmpIdx++) < tmp->entry->data_size_or_child_count) {
            build_node_tree_recursive(handle, tmp);
        }
    }


    current_node = tmp;
}

void build_node_tree(pak_handle_t *handle) {
    assert(handle);
    assert(handle->header);

    current_node = handle->root;

    while (current_idx < handle->header->entry_count) {
        pak_node_t* tmp = pak_create_node();

        tmp->entry = pak_get_entry_from_index(handle, current_idx);
        strcpy(tmp->filename, pak_get_string_from_index(handle, current_idx));
        tmp->is_dir = PAK_ENTRY_IS_DIR(tmp->entry);
        current_idx++;

        current_node->next = tmp;
        tmp->prev = current_node;
        current_node = tmp;

        if (tmp->is_dir) {
            int64_t tmpIdx = 0;
            while ((tmpIdx++) < tmp->entry->data_size_or_child_count) {
                build_node_tree_recursive(handle, tmp);
            }
        }

        current_node = tmp;
    }

    current_node->next = NULL;
}

static char curpath[FILENAME_MAX] = {0};
pak_node_t* find_recursive(pak_node_t* node, const char* filename) {
    assert(node);
    pak_node_t* ret = NULL;
    char tmp[FILENAME_MAX];
    strcpy(tmp, curpath);
    strcat(curpath, "/");
    strcat(curpath, node->filename);

    if (!strcmp(curpath, filename))
        ret = node;

    if (ret == NULL && node->is_dir) {
        pak_node_t* child = node->child;
        while (child != NULL) {
            ret = find_recursive(child, filename);
            if (ret)
                break;

            child = child->next;
        }
    }

    strcpy(curpath, tmp);

    return ret;
}

pak_node_t* pak_find_file(pak_handle_t* handle, const char* filepath) {
    assert(handle);
    pak_node_t* node = handle->root;
    pak_node_t* ret = NULL;
    while (node != NULL) {
        ret = find_recursive(node, filepath);
        if (ret && !ret->is_dir)
            break;

        node = node->next;
    }

    return ret;
}


pak_node_t* pak_find_dir(pak_handle_t* handle, const char* path) {
    assert(handle);
    pak_node_t* node = handle->root;
    pak_node_t* ret = NULL;
    while (node != NULL) {
        ret = find_recursive(node, path);
        if (ret && ret->is_dir)
            break;

        node = node->next;
    }

    return ret;
}

pak_node_t* pak_find(pak_handle_t* handle, const char* filepath) {
    assert(handle);
    pak_node_t* node = handle->root;
    pak_node_t* ret = NULL;
    while (node != NULL) {
        ret = find_recursive(node, filepath);
        if (ret)
            break;

        node = node->next;
    }

    return ret;
}

pak_file_t* pak_create_file() {
    pak_file_t* ret = pak_alloc(sizeof(pak_file_t*));
    memset(ret, 0, sizeof(pak_file_t));
    return ret;
}

void pak_free_file(pak_file_t* file) {
    pak_free(file);
}

pak_file_t* pak_open_file(pak_handle_t* handle, const char* filepath) {
    assert(handle);
    pak_file_t* ret = pak_create_file();
    ret->handle = handle;
    ret->node = pak_find_file(handle, filepath);
    if (!ret)
    {
        pak_free_file(ret);
        ret = NULL;
    }
    return ret;
}

void pak_close_file(pak_file_t* handle) {
    assert(handle);
    pak_free_file(handle);
}

uint32_t pak_file_read_uint(pak_file_t* file) {
    assert(file);
    pak_handle_t* handle = file->handle;
    uint32_t ret;
    fseek(handle->file, handle->header->data_offset + file->node->entry->data_offset_or_first_child + file->position, SEEK_SET);
    fread(&ret, 1, sizeof(uint32_t), handle->file);

    return ret;
}

pak_node_t* pak_create_node() {
    pak_node_t* ret = pak_alloc(sizeof(pak_node_t));
    assert(ret);
    memset(ret, 0, sizeof(pak_node_t));
    ret->child = NULL;
    ret->entry = NULL;
    ret->is_dir = false;
    ret->next = NULL;
    ret->prev = NULL;
    ret->parent = NULL;
    return ret;
}

void pak_free_node(pak_node_t* node) {
    assert(node);
    if (node->next)
        pak_free_node(node->next);
    if (node->child)
        pak_free_node(node->child);
    memset(node, 0, sizeof(pak_node_t));

    pak_free(node);
}


size_t pak_file_seek(pak_file_t* file, int64_t offset, int whence) {
    assert(file);

    switch (whence) {
        case SEEK_SET: {
            if (!(offset >= 0 && offset <= file->node->entry->data_size_or_child_count))
                return -1;

            file->position = offset;
            break;
        }
        case SEEK_CUR: {
            if (!(file->position + offset >= 0 && file->position + offset <= file->node->entry->data_size_or_child_count))
                return -1;

            file->position += offset;
            break;
        }
        case SEEK_END: {
            if (!((int64_t)(file->node->entry->data_size_or_child_count - offset) >= 0 && (int64_t)(file->node->entry->data_size_or_child_count - offset) <= file->node->entry->data_size_or_child_count))
                return -1;

            file->position = file->node->entry->data_size_or_child_count - offset;
            break;
        }
    }

    return file->position;
}
