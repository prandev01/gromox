#pragma once
#include <cstdint>
#include <cstdio>
#include <memory>
#include <gromox/defs.h>
#include <gromox/fileio.h>

struct LIST_FILE {
	~LIST_FILE();
	void *get_list() { return pfile; }
	int get_size() { return item_num; }

	std::unique_ptr<FILE, gromox::file_deleter> file_ptr;
    char        format[32];
    int         type_size[32];
    int         type_num;
    int         item_size;
    int         item_num;
    void*       pfile;
};

enum {
	EMPTY_ON_ABSENCE = 0,
	ERROR_ON_ABSENCE,
};

struct EXMDB_ITEM {
	std::string prefix, host;
	uint16_t port;
	enum {
		EXMDB_PRIVATE,
		EXMDB_PUBLIC,
	} type;
};

extern GX_EXPORT std::unique_ptr<LIST_FILE> list_file_initd(const char *filename, const char *sdlist, const char *format, unsigned int mode = EMPTY_ON_ABSENCE);
extern GX_EXPORT int list_file_read_fixedstrings(const char *filename, const char *sdlist, std::vector<std::string> &out);
extern GX_EXPORT int list_file_read_exmdb(const char *filename, const char *sdlist, std::vector<EXMDB_ITEM> &out);
