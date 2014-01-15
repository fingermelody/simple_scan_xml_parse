
enum _state    // 分析状态
{
    st_idle,        // 开始
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

    st_end,        // 分析结束
};
