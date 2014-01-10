#ifndef SIMP_SCAN_PARSE_H
#define SIMP_SCAN_PARSE_H

enum simple_state{
	st_Content,
	st_LT,
	st_End_Tag,
	st_PI_Comment_CDATA,
	st_Statrt_Tag,
	st_Alt_Val,
	st_Empty_Tag,
};

typedef struct _tag{
	int location;
	int lengh;
}Tag;

#endif
