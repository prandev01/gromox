#ifndef _H_ADMIN_UI_
#define _H_ADMIN_UI_
#include "common_types.h"

void admin_ui_init(int valid_days, const char *url_link,
	const char *cidb_path, const char *resource_path);
extern int admin_ui_run(void);
extern int admin_ui_stop(void);
extern void admin_ui_free(void);

#endif

