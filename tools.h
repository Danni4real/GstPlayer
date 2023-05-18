#ifndef TOOLS_H
#define TOOLS_H

#include <string>

int         to_int(std::string str);
double      to_double(std::string str);
bool        equal(std::string str_1, std::string str_2);
std::string extract_field(std::string str, int index);

std::string gen_message(std::string head, long neck);
std::string gen_message(std::string head, std::string neck);
std::string gen_message(std::string head, std::string neck, int         body);
std::string gen_message(std::string head, std::string neck, double      body);
std::string gen_message(std::string head, std::string neck, std::string body);

#endif // TOOLS_H
