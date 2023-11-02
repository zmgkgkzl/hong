/* event queue callbacks */
. = ALIGN(4);
_event_queue_observer_t_list_start = .;
KEEP(*(SORT_BY_NAME("._event_queue_observer_t.*")));
_event_queue_observer_t_list_end = .;
