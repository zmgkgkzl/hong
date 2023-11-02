#pragma once

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/__assert.h>

#define CEIL_DIV(a, b)  ((((a) - 1) / (b)) + 1)


////////////////////////////////////////////////////////////////////////////////////////
#define STRUCT_SECTION_ITERABLE_ALTERNATE2(secname, struct_type, varname) \
	TYPE_SECTION_ITERABLE(struct_type, varname, secname, varname)

#define STRUCT_SECTION_ITERABLE2(struct_type, varname) \
	STRUCT_SECTION_ITERABLE_ALTERNATE2(struct_type, struct_type, varname)


#define STRUCT_SECTION_FOREACH_ALTERNATE2(secname, struct_type, iterator) \
	TYPE_SECTION_FOREACH(struct_type, secname, iterator)

#define STRUCT_SECTION_FOREACH2(struct_type, iterator) \
	STRUCT_SECTION_FOREACH_ALTERNATE2(struct_type, struct_type, iterator)

////////////////////////////////////////////////////////////////////////////////////////
#define UNUSED_VARIABLE(X)  ((void)(X))
#define UNUSED_PARAMETER(X) UNUSED_VARIABLE(X)
#define UNUSED_RETURN_VALUE(X) UNUSED_VARIABLE(X)


////////////////////////////////////////////////////////////////////////////////////////
#define case_str(name) case name : return #name;

////////////////////////////////////////////////////////////////////////////////////////
#define BREAK_IF_ERROR(err_code) \
if ((err_code) != 0) \
{ \
	LOG_ERR(__FILE__); \
    LOG_ERR(" - ERROR. line: %d, with error code %d \r\n", __LINE__, err_code); \
    break; \
}

#define CONTINUE_IF_ERROR(err_code) \
if ((err_code) != 0) \
{ \
	LOG_ERR(__FILE__); \
    LOG_ERR(" - ERROR. line: %d, with error code %d \r\n", __LINE__, err_code); \
    continue; \
}

#define RETURN_IF_ERROR(err_code) \
if ((err_code) != 0) \
{ \
	LOG_ERR(__FILE__); \
    LOG_ERR(" - ERROR. line: %d, with error code %d \r\n", __LINE__, err_code); \
    return (err_code); \
}

#define REPORT_IF_ERROR(err_code) \
if ((err_code) != 0) \
{ \
	LOG_ERR(__FILE__); \
    LOG_ERR(" - ERROR. line: %d, with error code %d \r\n", __LINE__, err_code); \
}

#define NULL_PARAM_CHECK(param)                                                                    \
        if ((param) == NULL)                                                                       \
        {                                                                                          \
			LOG_ERR(__FILE__); \
			LOG_ERR(" - NULL. line: %d\r\n", __LINE__); \
            return -EINVAL;                                                                 \
        }
