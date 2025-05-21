/*
**
**
**
**
*/

#include "../include/myftp.hpp"

bool is_num(std::string &str)
{
    for (char c : str)
        if (!std::isdigit(c))
            return false;
    return true;
}


int main(int ac, char *av[])
{
    if (ac != 3)
        return 84;
    std::string temp (av[1]); 
    if (!is_num(temp))
        return 84;
    ServeurFTP a((unsigned int)atoi(av[1]), av[2]);
    if (!a.initialiser())
        return 84;
    a.demarrer();
}