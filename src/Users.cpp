/*
**
**
**
**
*/

#include "../include/myftp.hpp"    
#include "../include/Users.hpp"    


User::User() : estAuthentifie(false), modeAttentePasse(false), 
socketDonnees(-1), modeDonneesActif(false), portDataActif(0) 
{
}

User::~User()
{
    if (socketDonnees > 0) {
        close(socketDonnees);
    }
}