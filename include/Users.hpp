/*
**
**
**
**
*/

#pragma once

#include <string>

class User {
    public:
        std::string cheminCourant;
        std::string utilisateur;
        std::string adresseDataActif;
        bool estAuthentifie;
        bool modeAttentePasse;
        bool modeDonneesActif;
        int socketDonnees;
        int portDataActif;

        User();
        ~User();
};