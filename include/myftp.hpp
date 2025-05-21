/*
**
**
**
**
*/
#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cerrno>
#include "Users.hpp"

#define PORT 21


class ServeurFTP {
    private:
        int socketServeur;
        int port;
        std::vector<pollfd> descripteurs;
        std::string repertoireRacine;
        bool enFonctionnement;
        std::map<int, User *> clients;
        std::map<std::string, std::string> comptes; // (nom d'utilisateur -> mot de passe)
        
        static const int TAILLE_BUFFER = 1024;
        char buffer[TAILLE_BUFFER];
        
    public:
        ServeurFTP(int port, const std::string &racine);
        ~ServeurFTP();
        
        bool initialiser();
        
        void demarrer();
        
        void arreter();
        
    private:
        void initialiserComptes();
        
        void boucleServeur();
        
        void gererNouvelleConnexion();
        
        void gererClient(int socketClient);
        
        void fermerClient(int socketClient);
        
        void traiterCommande(int socketClient, const std::string& commandeRaw);

        void envoyerReponse(int socketClient, const std::string& reponse);
};