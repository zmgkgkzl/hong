/* menu callbacks */
. = ALIGN(4);
_menu_config_t_list_start = .;
KEEP(*(SORT_BY_NAME("._menu_config_t.*")));
_menu_config_t_list_end = .;
