#pragma once

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

typedef std::vector<std::string> ARRSTR;
int          strsplit(const std::string& s, ARRSTR& data, bool bSkipComments=true);
std::string& strltrim(std::string& str, const std::string& chars = "\t\n\v\f\r ");
std::string& strrtrim(std::string& str, const std::string& chars = "\t\n\v\f\r ");
std::string& strtrim(std::string& str, const std::string& chars = "\t\n\v\f\r ");

//cat non-overlapped strings
size_t strcat_s(char* dst, size_t dst_size, const char* src);
size_t strncat_s(char* dst, size_t dst_size, const char* src, size_t cnt);
size_t strcpy_s(char* dst, size_t dst_size, const char* src);

class CTextEditor{
    ARRSTR text;
    std::string filepath;
    bool is_loaded;
    bool is_modified;
  public:
    CTextEditor(const char* path);
    ~CTextEditor();
    void add(const char* line);
    void remove(uint32_t line_num);
    void replace(uint32_t line_num, const char* new_text);
    void print(void);
    void write(void);
    size_t size(void) {return text.size(); }
    bool isLoaded(void) { return is_loaded; }
    std::string get(size_t idx);
};
