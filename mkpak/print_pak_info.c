#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "pak.h"

static char curpath[FILENAME_MAX] = {0};

void print_name_recursive(pak_node_t* node) {
    char tmp[FILENAME_MAX];
    strcpy(tmp, curpath);
    strcat(curpath, "/");
    strcat(curpath, node->filename);
    printf("%s", curpath);
    if (node->parent) {
        printf(" (parent: %s, file id: %" PRIu64 ")\n", node->parent->filename, node->entry->file_id);
    } else {
        printf(" (file id: %" PRIu64 ")\n", node->entry->file_id);
    }

    if (node->is_dir) {

        pak_node_t* child = node->child;
        while (child != NULL) {
            print_name_recursive(child);
            child = child->next;
        }
    }

    strcpy(curpath, tmp);
}

void print_pak_info(const char* input) {
    pak_handle_t* pak = pak_open_read(input);

    if (pak) {
        pak_node_t* node = pak->root->next;
        while (node != NULL) {
            print_name_recursive(node);
            node = node->next;
        }

        printf("%" PRIu64 " Files in %s\n", pak_get_entry_count(pak), input);
        printf("Target endian is %s\n", (pak_get_endian(pak) ? "Big" : "Little"));
        printf("Entry table starts at 0x%.8" PRIX64 "\n", pak_get_entry_start(pak));
        printf("String table starts at 0x%.8" PRIX64 "\n", pak_get_string_table_offset(pak));
        printf("String table is %" PRIu64 " bytes long\n", pak_get_string_table_size(pak));
        printf("Data table starts at 0x%.8" PRIX64 "\n", pak_get_data_offset(pak));
        pak_close(pak);
    } else {
        exit(EXIT_FAILURE);
    }
}
