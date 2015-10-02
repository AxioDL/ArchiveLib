#include "pak.h"
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <inttypes.h>
#include "util.h"

static char curpath[FILENAME_MAX] = {0};

void dump_node_recursive(pak_node_t* node, pak_handle_t* pak, bool verbose) {
    return;
    char tmp[4096];
    strcpy(tmp, curpath);
    strcat(curpath, "/");
    strcat(curpath, node->filename);
    if (verbose)
        printf("%s\n", curpath);

    if (node->is_dir) {
        mkdir(node->filename, 0755);
        chdir(node->filename);
        pak_node_t* child = node->child;
        while (child) {
            dump_node_recursive(child, pak, verbose);
            child = child->next;
        }
        chdir("..");
    } else {
        char* buf = malloc(node->entry->data_size_or_child_count);
        fseeko64(pak->file, pak->header->data_offset + node->entry->data_offset_or_first_child, SEEK_SET);
        fread(buf, 1, node->entry->data_size_or_child_count, pak->file);
        FILE* out = fopen(node->filename, "wb");
        size_t data_len = node->entry->data_size_or_child_count;
        if (node->entry->flags & PAK_ENTRY_FLAGS_COMPRESSED) {
            void* uncmp = malloc(node->entry->data_uncompressed_size * 2);
            int64_t uncomp_len = util_decompress(buf, node->entry->data_size_or_child_count, uncmp, node->entry->data_uncompressed_size * 2);
            if (uncomp_len == node->entry->data_uncompressed_size) {
                free(buf);
                buf = uncmp;
                data_len = uncomp_len;
            } else {
                free(uncmp);
            }
        }
        fwrite(buf, 1, data_len, out);
        fclose(out);
        free(buf);
    }
    strcpy(curpath, tmp);
}

void dump_pak(char* input, char* output, bool verbose) {
    pak_handle_t* pak = pak_open_read(input);
    if (pak) {
        mkdir(output, 0755);
        chdir(output);
        strcat(curpath, output);

        /*
        // iterate through all the children of <root>
        pak_node_t* node = pak->root->next;
        while (node) {
            dump_node_recursive(node, pak, verbose);
            node = node->next;
        }

        printf("Dumped %" PRIu64 " files\n", pak_get_entry_count(pak));
        */
        pak_close(pak);
        exit(EXIT_SUCCESS);
    }
    exit(EXIT_FAILURE);
}
