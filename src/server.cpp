/*
**
**
**
**
*/

#include "../include/myftp.hpp"

ServeurFTP::ServeurFTP(int port = 21, const std::string & racine = "/tmp/ftp") 
        : port(port), repertoireRacine(racine), enFonctionnement(false)
{
    if (!std::filesystem::exists(repertoireRacine)) {
        std::filesystem::create_directories(repertoireRacine);
    }
    
    initialiserComptes();
}

ServeurFTP::~ServeurFTP()
{
    arreter();
}

bool ServeurFTP::initialiser()
{
    socketServeur = socket(AF_INET, SOCK_STREAM, 0);
    if (socketServeur < 0) {
        std::cerr << "Erreur lors de la création du socket: " << strerror(errno) << std::endl;
        return false;
    }

    struct sockaddr_in adresseServeur;
    adresseServeur.sin_family = AF_INET;
    adresseServeur.sin_addr.s_addr = INADDR_ANY;
    adresseServeur.sin_port = htons(port);

    if (bind(socketServeur, (struct sockaddr*)&adresseServeur, sizeof(adresseServeur)) < 0) {
        std::cerr << "Erreur lors du binding: " << strerror(errno) << std::endl;
        close(socketServeur);
        return false;
    }

    if (listen(socketServeur, 10) < 0) {
        std::cerr << "Erreur lors de la mise en écoute: " << strerror(errno) << std::endl;
        close(socketServeur);
        return false;
    }

    pollfd pfd;
    pfd.fd = socketServeur;
    pfd.events = POLLIN;
    descripteurs.push_back(pfd);

    std::cout << "Serveur FTP initialisé sur le port " << port << std::endl;
    std::cout << "Répertoire racine: " << repertoireRacine << std::endl;
    return true;
}

void ServeurFTP::demarrer()
{
    if (!enFonctionnement) {
        enFonctionnement = true;
        boucleServeur();
    }
}

void ServeurFTP::arreter() 
{
    enFonctionnement = false;
    
    for (auto& client : clients) {
        close(client.first);
        delete client.second;
    }
    
    if (socketServeur >= 0) {
        close(socketServeur);
        socketServeur = -1;
    }
    
    clients.clear();
    descripteurs.clear();
}

