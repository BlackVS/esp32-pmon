#include "app.h"

static const char *TAG = __FILE__;

//cat non-overlapped strings, slow
size_t strcat_s(char* dst, size_t dst_size, const char* src)
{
    if(!dst) return 0;
    if(!src) return 0;
    if(!dst_size) return 0;
    size_t src_len=strlen(src);
    size_t dst_len=strlen(dst);
    size_t chars2cat=dst_size-1-src_len-dst_len;
    if(chars2cat) {
        strncat(dst,src,chars2cat);
    }
    return strlen(dst);
}

//cat non-overlapped strings, slow
size_t strncat_s(char* dst, size_t dst_size, const char* src, size_t cnt)
{
    if(!dst) return 0;
    if(!src) return 0;
    if(!dst_size) return 0;
    size_t src_len=strlen(src);
    size_t dst_len=strlen(dst);
    size_t chars2cat=dst_size-1-src_len-dst_len;
    if(chars2cat<=0) 
        return 0;
    chars2cat=MIN(chars2cat, cnt);
    if(chars2cat) {
        strncat(dst,src,chars2cat);
    }
    return strlen(dst);
}


size_t strcpy_s(char* dst, size_t dst_size, const char* src)
{
    if(!dst) return 0;
    if(!src) return 0;
    if(!dst_size) return 0;
    memset(dst, 0, dst_size);
    size_t src_len=strlen(src);
    size_t chars2cpy=MIN(dst_size-1, src_len);
    if(chars2cpy) {
        strncpy(dst,src,chars2cpy);
    }
    return strlen(dst);
}



std::string& strltrim(std::string& str, const std::string& chars)
{
    str.erase(0, str.find_first_not_of(chars));
    return str;
}
 
std::string& strrtrim(std::string& str, const std::string& chars)
{
    str.erase(str.find_last_not_of(chars) + 1);
    return str;
}
 
std::string& strtrim(std::string& str, const std::string& chars)
{
    return strltrim(strrtrim(str, chars), chars);
}

int strsplit(const std::string& s, ARRSTR& data, bool bSkipComments)
{
    size_t pos=0;
    size_t len=s.size();
    std::string t;
    data.clear();
    while(pos<len){
      char c=s[pos];
      if(c=='\n'||c=='\r')
      {
        if(bSkipComments)
        {
            if(t.size()>1&&t[0]!='#')
                data.push_back(t);
        } else
          data.push_back(t);
        t.clear();
        pos++;
        //skip
        while(pos<len&&(s[pos]=='\n'||s[pos]=='\r'))
          pos++;
      } else 
      {
        t+=c;
        pos++;
      }
    }
    return data.size();
}

CTextEditor::CTextEditor(const char* path) 
{
    is_loaded=false;
    is_modified=false;
    if(path) {
        filepath.assign(path);
        std::ifstream fs;
        fs.open(path);
        std::stringstream strStream;
        strStream << fs.rdbuf(); //read the file
        std::string str = strStream.str(); //str holds the content of the file
        strsplit(str, text);
        fs.close();
        is_loaded=true;
    }
}

CTextEditor::~CTextEditor(){
    if(is_modified){
        write();
    }
}

void CTextEditor::add(const char* line){
    text.push_back(line);
    is_modified=true;
}

void CTextEditor::remove(uint32_t line_num)
{
    if(line_num>=text.size()) {
    printf("Error: wrong line number specified!");
    return;
    }
    text.erase( text.begin()+line_num);
    is_modified=true;
}

void CTextEditor::replace(uint32_t line_num, const char* new_text)
{
    if(line_num>=text.size()) {
    printf("Error: wrong line number specified!");
    return;
    }
    text[line_num]=new_text;
    is_modified=true;
}

void CTextEditor::print(void)
{
    if(text.size()==0){
    printf("No script found!\n");
    return;
    }
    printf("--------------------------------------------------------------------------------------\n");
    printf("File : %s\n", filepath.c_str());
    printf("--------------------------------------------------------------------------------------\n");
    int id=0;
    for(auto& s: text){
    printf("%2i : %s\n", id, s.c_str());
    id++;
    }
    printf("--------------------------------------------------------------------------------------\n");
}

void CTextEditor::write(void)
{
    if(filepath.size()==0) {
    printf("Error: no filepath specified!");
    return;
    }
    std::ofstream fs(filepath, std::ofstream::out | std::ios_base::trunc);
    if(fs.is_open())
    {
    for(auto& s: text){
        fs << s << std::endl;
    }
    fs.close();
    is_modified=false;
    } else {
    ESP_LOGE(TAG, "Failed to write to %s!", filepath.c_str()); 
    perror("Reason:");
    }
}

std::string CTextEditor::get(size_t idx)
{
    if(idx<text.size()){
        return text[idx];
    }
    return "";
}
