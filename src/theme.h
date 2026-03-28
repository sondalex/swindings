#ifndef THEME_H
#define THEME_H


typedef struct {
    char *color;

} Theme;


Status theme_set_from_config(Theme *theme);

static Status theme_parse_file(char *filepath);
static char * theme_get_config_filepath();

#endif
