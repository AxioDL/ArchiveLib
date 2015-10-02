#include "pak.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <zlib.h>
#include "util.h"
#include <time.h>
#include <inttypes.h>

static char curpath[FILENAME_MAX] = {0};
static int time_last = 0;

int pak_file_or_dir(struct dirent* dent, uint32_t idx, FILE* entryFile, FILE* stringFile, FILE* dataFile, bool compress, bool verbose)
{
    char tmppath[FILENAME_MAX];
    memcpy(tmppath, curpath, strlen(curpath) + 1);
    strcat(curpath, "/");
    strcat(curpath, dent->d_name);

    if ((time(NULL) - time_last) > 1 && !verbose)
    {
        printf(".");
        fflush(stdout);
        time_last = time(NULL);
    }

    struct stat64 st;
    stat64(curpath, &st);
    if (verbose)
        printf("%s\n", curpath);

    pak_entry_t* entry = pak_create_entry();

    if (S_ISLNK(st.st_mode))
        goto fail;

    if (!S_ISDIR(st.st_mode)) {

        FILE* in = fopen(curpath, "rb");
        if (in) {
            entry->flags = PAK_ENTRY_FLAGS_WRITEABLE;
            entry->file_id = idx;
            entry->string_offset = ftello64(stringFile);
            entry->data_offset_or_first_child = ftello64(dataFile);
            entry->data_size_or_child_count = st.st_size;
            entry->data_uncompressed_size = 0;

            idx++;

            fwrite(dent->d_name, 1, strlen(dent->d_name) + 1, stringFile);
            uint32_t data_len = st.st_size;
            char* buf = malloc(st.st_size);
            fread(buf, 1, st.st_size, in);

            if (compress) {
                void* comp_buf = malloc(data_len);
                size_t comp_len = util_compress(buf, st.st_size, comp_buf, Z_BEST_COMPRESSION);
                if (comp_len < (size_t)st.st_size) {
                    entry->flags |= PAK_ENTRY_FLAGS_COMPRESSED;
                    entry->data_uncompressed_size = st.st_size;
                    entry->data_size_or_child_count = comp_len;
                    data_len = (comp_len + 31) & ~31;
                    free(buf);
                    buf = malloc(data_len);
                    pak_clear(buf, data_len);
                    memcpy(buf, comp_buf, comp_len);
                    free(comp_buf);
                } else {
                    data_len = (data_len + 31) & ~31;
                    void* tmp = malloc(data_len);
                    pak_clear(tmp, data_len);
                    memcpy(tmp, buf, st.st_size);
                    free(buf);
                    buf = tmp;
                }
            }

            fwrite(entry, 1, sizeof(pak_entry_t), entryFile);
            fclose(in);
            fwrite(buf, 1, data_len, dataFile);
            free(buf);
        }

    } else if (S_ISDIR(st.st_mode)) {
        entry->flags = PAK_ENTRY_FLAGS_WRITEABLE | PAK_ENTRY_FLAGS_DIR;
        entry->file_id = idx;
        entry->string_offset = ftello64(stringFile);
        entry->data_offset_or_first_child = idx + 1;
        entry->data_size_or_child_count = 0;
        idx++;

        fwrite(dent->d_name, 1, strlen(dent->d_name) + 1, stringFile);
        uint32_t offset = ftello64(entryFile);
        fseeko64(entryFile, sizeof(pak_entry_t), SEEK_CUR);
        DIR* thisDir = opendir(curpath);
        if (thisDir) {   struct dirent * child = NULL;
            while ((child = readdir(thisDir)) != NULL) {
                if (!strcmp(child->d_name, ".") || !strcmp(child->d_name, "..") || !strcmp(child->d_name, ".git"))
                    continue;

                idx = pak_file_or_dir(child, idx, entryFile, stringFile, dataFile, compress, verbose);
                entry->data_size_or_child_count++;
            }
            if (!entry->data_size_or_child_count)
                entry->data_offset_or_first_child = 0;
        }

        fseeko64(entryFile, offset, SEEK_SET);
        fwrite(entry, 1, sizeof(pak_entry_t), entryFile);
        fseeko64(entryFile, 0, SEEK_END);

    }
    else if (verbose)
fail:
        printf("skipped\n");

    strcpy(curpath, tmppath);
    pak_free_entry(entry);

    return idx;
}

void make_pak(char *input, char *output, bool compress, bool verbose) {

    time_last = time(NULL);
    DIR* dir;
    struct dirent * dent;
    char* basepath = input;
    dir = opendir(basepath);
    if (!dir) {
        exit(EXIT_FAILURE);
    }

    pak_handle_t* handle = pak_open_write(output);

    printf("Building entry, string and data files...");
    fflush(stdout);

    char dataTempNam[FILENAME_MAX];
    gen_random(dataTempNam, 44);
    char dataTempPath[FILENAME_MAX];
    memset(dataTempPath, 0, FILENAME_MAX);
    sprintf(dataTempPath, "/tmp/%s.data", dataTempNam);
    char stringTempPath[FILENAME_MAX];
    memset(stringTempPath, 0, FILENAME_MAX);
    sprintf(stringTempPath, "/tmp/%s.string", dataTempNam);
    char entryTempPath[FILENAME_MAX];
    memset(entryTempPath, 0, FILENAME_MAX);
    sprintf(entryTempPath, "/tmp/%s.entry", dataTempNam);
    FILE* dataFile = fopen(dataTempPath, "wb");
    FILE* entryFile  = fopen(entryTempPath, "wb");
    FILE* stringFile = fopen(stringTempPath, "wb");

    uint64_t idx = 0;
    while ((dent = readdir(dir)) != NULL)
    {
        if ((time(NULL) - time_last) > 1)
        {
            printf(".");
            fflush(stdout);
            time_last = time(NULL);
        }
        if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
            continue;

        curpath[0] = '\0';
        strcat(curpath, basepath);
        idx = pak_file_or_dir(dent, idx, entryFile, stringFile, dataFile, compress, verbose);
    }

    printf("\nBuilding pak...\n");
    closedir(dir);
    fclose(dataFile);
    fclose(stringFile);
    fclose(entryFile);

    // now to build the file
    // first reopen the temp data
    struct stat64 st;
    entryFile = fopen(entryTempPath, "rb");
    stringFile = fopen(stringTempPath, "rb");
    dataFile = fopen(dataTempPath, "rb");

    // read entry table
    stat64(entryTempPath, &st);
    size_t entryTableSize = st.st_size;
    size_t entryCount = entryTableSize / sizeof(pak_entry_t);
    char* entryTableBuf = malloc(entryTableSize);
    fread(entryTableBuf, 1, entryTableSize, entryFile);

    // read string table
    stat64(stringTempPath, &st);
    size_t stringTableSize = st.st_size;
    char* stringTableBuf = malloc(stringTableSize);
    fread(stringTableBuf, 1, stringTableSize, stringFile);

    // read data table
    stat64(dataTempPath, &st);
    size_t dataTableSize = st.st_size;

    // close files
    fclose(entryFile);
    fclose(stringFile);

    // TODO: Make this manageable
    pak_set_entry_start(handle, (sizeof(pak_header_t) + 31) & ~31);
    pak_set_entry_count(handle, entryCount);
    pak_set_string_table_offset(handle, (handle->header->entry_start + entryTableSize + 31) & ~31);
    pak_set_string_table_size(handle, stringTableSize);
    pak_set_data_offset(handle, (handle->header->string_table_offset + stringTableSize + 31) & ~31);

    // pad buffers
    size_t paddedEntryBufSize = (entryTableSize + 31) & ~31;
    char* paddedEntryBuf = malloc(paddedEntryBufSize);
    pak_clear(paddedEntryBuf, paddedEntryBufSize);
    memcpy(paddedEntryBuf, entryTableBuf, entryTableSize);
    free(entryTableBuf);

    size_t paddedStringBufSize = (stringTableSize + 31) & ~31;
    char* paddedStringBuf = malloc(paddedStringBufSize);

    pak_clear(paddedStringBuf, paddedStringBufSize);
    memcpy(paddedStringBuf, stringTableBuf, stringTableSize);
    free(stringTableBuf);

    // write pak
    FILE* pak = handle->file;
    if (pak)
    {
        fwrite(handle->header, 1, sizeof(pak_header_t), pak);
        fseeko64(pak, (sizeof(pak_header_t) + 31) & ~31, SEEK_SET);
        fwrite(paddedEntryBuf, 1, paddedEntryBufSize, pak);
        fwrite(paddedStringBuf, 1, paddedStringBufSize, pak);

        size_t bytesRead = 0;
        size_t blockSize = BUF_SIZ;
        while(bytesRead < dataTableSize)
        {
            if (blockSize > dataTableSize - bytesRead)
                blockSize = dataTableSize - bytesRead;
            char buf[blockSize];
            size_t nextReadSize = fread(buf, 1, blockSize, dataFile);
            if (nextReadSize <= 0)
                break;  // error or early EOF!

            fwrite(buf, 1, nextReadSize, pak);
            bytesRead += nextReadSize;
        }
    }

    fclose(dataFile);

    printf("Stored %" PRIu64 " files (%s)\n", pak_get_entry_count(handle), (compress ? "compressed" : "uncompressed"));
    pak_close(handle);
    free(paddedEntryBuf);
    free(paddedStringBuf);
    remove(entryTempPath);
    remove(stringTempPath);
    remove(dataTempPath);
}
