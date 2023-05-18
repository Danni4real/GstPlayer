#include <string>

#include "tools.h"

std::string extract_field(std::string str, int index)
{
    int pos;
    std::string field;

    while(index >= 0)
    {
        pos   = str.find(',');
        field = str.substr(0, pos);
        str   = str.substr(pos+1);

        index--;
    }

    return field;
}

bool equal(std::string str_1, std::string str_2)
{
    if(str_1 == str_2)
        return true;

    return false;
}

int to_int(std::string str)
{
    return atoi(str.c_str());
}

double to_double(std::string str)
{
    return atof(str.c_str());
}

std::string gen_message(std::string head, long neck)
{
    return head+","+std::to_string(neck);
}

std::string gen_message(std::string head, std::string neck)
{
    return head+","+neck;
}

std::string gen_message(std::string head, std::string neck, int body)
{
    return head+","+neck+","+std::to_string(body);
}

std::string gen_message(std::string head, std::string neck, double body)
{
    return head+","+neck+","+std::to_string(body);
}

std::string gen_message(std::string head, std::string neck, std::string body)
{
    return head+","+neck+","+body;
}
