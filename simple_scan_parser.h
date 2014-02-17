#ifndef SIMPLE_SCAN_PARSE_H
#define SIMPLE_SCAN_PARSE_H
enum _state    // ����״̬
{
    st_idle,        // ��ʼ
    st_lt,
    st_txt,
    st_start_tag,
    st_end_tag,
    st_comment,
    st_cdata,
    st_instruction,
    st_attribute_pre,
    st_attribute_name,
    st_attribute_value_pre,
    st_attribute_value,

    st_end,        // ��������
};

#endif
