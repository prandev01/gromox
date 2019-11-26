#ifndef _H_WHITELIST_UI_
#define _H_WHITELIST_UI_

void whitelist_ui_init(const char *list_path, const char *mount_path,
	const char *url_link, const char *resource_path);
extern int whitelist_ui_run(void);
extern int whitelist_ui_stop(void);
extern void whitelist_ui_free(void);

#endif

