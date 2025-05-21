/*
**
**
**
**
*/

#include "../include/myftp.hpp"


void ServeurFTP::initialiserComptes()
{
    comptes["Anonymous"] = "";

    comptes["admin"] = "admin";
}


void ServeurFTP::boucleServeur()
{
    std::cout << "Serveur FTP démarré. En attente de connexions..." << std::endl;
    
    while (enFonctionnement) {
        int nbEvents = poll(descripteurs.data(), descripteurs.size(), 1000); // Timeout de 1 seconde
        
        if (nbEvents < 0) {
            std::cerr << "Erreur lors du poll: " << strerror(errno) << std::endl;
            break;
        }
        
        for (size_t i = 0; i < descripteurs.size() && nbEvents > 0; ++i) {
            if (descripteurs[i].revents & POLLIN) {
                nbEvents--;
                
                if (descripteurs[i].fd == socketServeur) {
                    gererNouvelleConnexion();
                } else {
                    gererClient(descripteurs[i].fd);
                }
            }
        }
    }
    
    std::cout << "Serveur FTP arrêté." << std::endl;
}


void ServeurFTP::gererNouvelleConnexion()
{
    struct sockaddr_in adresseClient;
    socklen_t tailleClient = sizeof(adresseClient);
    
    int socketClient = accept(socketServeur, (struct sockaddr*)&adresseClient, &tailleClient);
    
    if (socketClient < 0) {
        std::cerr << "Erreur lors de l'acceptation d'une connexion: " << strerror(errno) << std::endl;
        return;
    }
    
    pollfd pfd;
    pfd.fd = socketClient;
    pfd.events = POLLIN;
    descripteurs.push_back(pfd);
    
    clients[socketClient] = new User();
    clients[socketClient]->cheminCourant = repertoireRacine;
    
    std::cout << "Nouvelle connexion acceptée depuis " 
              << inet_ntoa(adresseClient.sin_addr) << ":" 
              << ntohs(adresseClient.sin_port) << std::endl;
    
    std::string message = "220 Bienvenue sur le serveur FTP\r\n";
    write(socketClient, message.c_str(), message.length());
}


void ServeurFTP::gererClient(int socketClient)
{
    FILE* fp = fdopen(dup(socketClient), "r");
    char buffer[TAILLE_BUFFER];
    if (!fp) {
        std::cerr << "Erreur lors de la création du flux de données: " << strerror(errno) << std::endl;
        fermerClient(socketClient);
        return;
    }
    if (fgets(buffer, TAILLE_BUFFER - 1, fp) == nullptr) {
        if (feof(fp)) {
            std::cout << "Connexion fermée par le client" << std::endl;
        } else
            std::cerr << "Erreur de lecture du socket client: " << strerror(errno) << std::endl;
        fclose(fp);
        fermerClient(socketClient);
        return;
    }
    std::string commande(buffer);
    fclose(fp);    
    traiterCommande(socketClient, commande);
}


void ServeurFTP::fermerClient(int socketClient)
{
    std::cout << "Client déconnecté (socket " << socketClient << ")" << std::endl;
    
    if (clients[socketClient]->socketDonnees > 0) {
        close(clients[socketClient]->socketDonnees);
    }
    
    delete clients[socketClient];
    close(socketClient);
    clients.erase(socketClient);
    
    for (auto it = descripteurs.begin(); it != descripteurs.end(); ++it) {
        if (it->fd == socketClient) {
            descripteurs.erase(it);
            break;
        }
    }
}


void ServeurFTP::traiterCommande(int socketClient, const std::string& commandeRaw)
{
    std::string commande = commandeRaw;
    if (commande.back() == '\n')
        commande.pop_back();
    if (commande.back() == '\r')
        commande.pop_back();
    
    std::cout << "Commande reçue de socket " << socketClient << ": " << commande << std::endl;

    std::string nomCommande;
    std::string argument;
    
    size_t espacePos = commande.find(' ');
    if (espacePos != std::string::npos) {
        nomCommande = commande.substr(0, espacePos);
        argument = commande.substr(espacePos + 1);
    } else
        nomCommande = commande;
    
    for (char& c : nomCommande)
        c = toupper(c);
    
    User* client = clients[socketClient];
    
    if (nomCommande == "USER") {
        client->utilisateur = argument;
        client->modeAttentePasse = true;
        client->estAuthentifie = false;
        
        if (argument == "Anonymous") {
            envoyerReponse(socketClient, "331 Nom d'utilisateur Anonymous accepté, envoyez votre email comme mot de passe.");
        } else if (comptes.find(argument) != comptes.end()) {
            envoyerReponse(socketClient, "331 Nom d'utilisateur accepté, veuillez entrer le mot de passe.");
        } else {
            envoyerReponse(socketClient, "530 Utilisateur non reconnu.");
            client->modeAttentePasse = false;
        }
    } else if (nomCommande == "PASS") {
        if (client->modeAttentePasse) {
            if (client->utilisateur == "Anonymous" || 
                (comptes.find(client->utilisateur) != comptes.end() && 
                 comptes[client->utilisateur] == argument)) {
                client->estAuthentifie = true;
                envoyerReponse(socketClient, "230 Utilisateur connecté.");
            } else {
                envoyerReponse(socketClient, "530 Identifiants incorrects.");
            }
            client->modeAttentePasse = false;
        } else
            envoyerReponse(socketClient, "503 Entrez d'abord un nom d'utilisateur avec USER.");
    } else if (!client->estAuthentifie && (nomCommande != "QUIT")) {
        envoyerReponse(socketClient, "530 Veuillez vous authentifier d'abord.");
    } else if (nomCommande == "PWD") {
        std::string cheminRelatif = client->cheminCourant.substr(repertoireRacine.length());
        if (cheminRelatif.empty())
            cheminRelatif = "/";
        envoyerReponse(socketClient, "257 \"" + cheminRelatif + "\" est le répertoire courant.");
    } else if (nomCommande == "CWD") {
        std::string nouveauChemin = argument;
        if (nouveauChemin.empty()) {
            envoyerReponse(socketClient, "501 Paramètre requis.");
            return;
        }
        
        if (nouveauChemin[0] != '/') {
            nouveauChemin = client->cheminCourant + "/" + nouveauChemin;
        } else {
            nouveauChemin = repertoireRacine + nouveauChemin;
        }
        
        nouveauChemin = std::filesystem::path(nouveauChemin).lexically_normal().string();
        
        if (std::filesystem::exists(nouveauChemin) && 
            std::filesystem::is_directory(nouveauChemin) &&
            nouveauChemin.find(repertoireRacine) == 0) {
            client->cheminCourant = nouveauChemin;
            envoyerReponse(socketClient, "250 Directory changed to " + nouveauChemin);
        } else {
            envoyerReponse(socketClient, "550 Failed to change directory.");
        }
    } else if (nomCommande == "CDUP") {
        std::string nouveauChemin("..");
        if (nouveauChemin.empty()) {
            envoyerReponse(socketClient, "501 Paramètre requis.");
            return;
        }
        
        if (nouveauChemin[0] != '/') {
            nouveauChemin = client->cheminCourant + "/" + nouveauChemin;
        } else {
            nouveauChemin = repertoireRacine + nouveauChemin;
        }
        
        nouveauChemin = std::filesystem::path(nouveauChemin).lexically_normal().string();
        
        if (std::filesystem::exists(nouveauChemin) && 
            std::filesystem::is_directory(nouveauChemin) &&
            nouveauChemin.find(repertoireRacine) == 0) {
            client->cheminCourant = nouveauChemin;
            envoyerReponse(socketClient, "250 Directory changed to " + nouveauChemin);
        } else {
            envoyerReponse(socketClient, "550 Failed to change directory.");
        }
    } else if (nomCommande == "QUIT") {
        envoyerReponse(socketClient, "221 Au revoir.");
        fermerClient(socketClient);
    } else if (nomCommande == "DELE") {
        if (argument.empty()) {
            envoyerReponse(socketClient, "501 Paramètre requis.");
            return;
        }
        
        std::string cheminFichier = client->cheminCourant + "/" + argument;
        
        // Vérifier si le fichier existe et n'est pas un dossier
        if (std::filesystem::exists(cheminFichier) && 
            !std::filesystem::is_directory(cheminFichier)) {
            try {
                std::filesystem::remove(cheminFichier);
                envoyerReponse(socketClient, "250 File deleted.");
            } catch (const std::exception& e) {
                envoyerReponse(socketClient, "550 Failed to delete file: " + std::string(e.what()));
            }
        } else {
            envoyerReponse(socketClient, "550 File not found or not a regular file.");
        }
    } else if (nomCommande == "PORT") {
        // Implémenter la connexion en mode actif
        std::vector<std::string> parts;
        std::string delimiter = ",";
        size_t pos = 0;
        std::string token;
        std::string s = argument;
        while ((pos = s.find(delimiter)) != std::string::npos) {
            token = s.substr(0, pos);
            parts.push_back(token);
            s.erase(0, pos + delimiter.length());
        }
        parts.push_back(s);
        
        if (parts.size() == 6) {
            client->adresseDataActif = parts[0] + "." + parts[1] + "." + parts[2] + "." + parts[3];
            client->portDataActif = std::stoi(parts[4]) * 256 + std::stoi(parts[5]);
            client->modeDonneesActif = true;
            envoyerReponse(socketClient, "200 PORT command successful.");
        } else {
            envoyerReponse(socketClient, "501 Syntax error in parameters.");
        }
    } else if (nomCommande == "PASV") {
        // Implémenter la connexion en mode passif
        // Code pour créer un socket serveur temporaire et obtenir son port
        int socketPassif = socket(AF_INET, SOCK_STREAM, 0);
        if (socketPassif < 0) {
            envoyerReponse(socketClient, "425 Cannot open passive connection.");
            return;
        }
        
        struct sockaddr_in adressePassive;
        memset(&adressePassive, 0, sizeof(adressePassive));
        adressePassive.sin_family = AF_INET;
        adressePassive.sin_addr.s_addr = INADDR_ANY;
        adressePassive.sin_port = 0;  // Laisser le système choisir un port
        
        if (bind(socketPassif, (struct sockaddr*)&adressePassive, sizeof(adressePassive)) < 0) {
            close(socketPassif);
            envoyerReponse(socketClient, "425 Cannot open passive connection.");
            return;
        }
        
        if (listen(socketPassif, 1) < 0) {
            close(socketPassif);
            envoyerReponse(socketClient, "425 Cannot open passive connection.");
            return;
        }
        
        // Obtenir le port assigné
        socklen_t len = sizeof(adressePassive);
        if (getsockname(socketPassif, (struct sockaddr*)&adressePassive, &len) < 0) {
            close(socketPassif);
            envoyerReponse(socketClient, "425 Cannot open passive connection.");
            return;
        }
        
        int portPassif = ntohs(adressePassive.sin_port);
        int p1 = portPassif / 256;
        int p2 = portPassif % 256;
        
        client->socketDonnees = socketPassif;
        client->modeDonneesActif = false;
        
        // Construire la réponse PASV avec l'adresse IP locale et le port
        struct sockaddr_in adresseLocale;
        getsockname(socketServeur, (struct sockaddr*)&adresseLocale, &len);
        std::string ipLocale = inet_ntoa(adresseLocale.sin_addr);
        std::replace(ipLocale.begin(), ipLocale.end(), '.', ',');
        
        std::string reponse = "227 Entering Passive Mode (" + ipLocale + "," + std::to_string(p1) + "," + std::to_string(p2) + ").";
        envoyerReponse(socketClient, reponse);
    } else if (nomCommande == "LIST") {
        // Lister les fichiers
        std::string cheminListe = client->cheminCourant;
        if (!argument.empty() && argument != "-l" && argument != "-a") {
            if (argument[0] != '/') {
                cheminListe = client->cheminCourant + "/" + argument;
            } else {
                cheminListe = repertoireRacine + argument;
            }
            
            // Vérifier si le chemin existe
            if (!std::filesystem::exists(cheminListe)) {
                envoyerReponse(socketClient, "550 No such file or directory.");
                return;
            }
        }
        
        envoyerReponse(socketClient, "150 Here comes the directory listing.");
    } else if (nomCommande == "RETR") {
        // Télécharger un fichier
        if (argument.empty()) {
            envoyerReponse(socketClient, "501 Paramètre requis.");
            return;
        }
        
        std::string cheminFichier = client->cheminCourant + "/" + argument;
        
        // Vérifier si le fichier existe
        if (!std::filesystem::exists(cheminFichier) || std::filesystem::is_directory(cheminFichier)) {
            envoyerReponse(socketClient, "550 File not found or access denied.");
            return;
        }
        
        envoyerReponse(socketClient, "150 Opening data connection.");
        
        int socketData = -1;
        
        // Établir la connexion de données
        if (client->modeDonneesActif) {
            // Mode actif
            socketData = socket(AF_INET, SOCK_STREAM, 0);
            if (socketData < 0) {
                envoyerReponse(socketClient, "425 Cannot open data connection.");
                return;
            }
            
            struct sockaddr_in adresseData;
            memset(&adresseData, 0, sizeof(adresseData));
            adresseData.sin_family = AF_INET;
            adresseData.sin_port = htons(client->portDataActif);
            inet_pton(AF_INET, client->adresseDataActif.c_str(), &adresseData.sin_addr);
            
            if (connect(socketData, (struct sockaddr*)&adresseData, sizeof(adresseData)) < 0) {
                close(socketData);
                envoyerReponse(socketClient, "425 Cannot open data connection.");
                return;
            }
        } else {
            // Mode passif
            struct sockaddr_in adresseClientData;
            socklen_t tailleClientData = sizeof(adresseClientData);
            socketData = accept(client->socketDonnees, (struct sockaddr*)&adresseClientData, &tailleClientData);
            close(client->socketDonnees);
            client->socketDonnees = -1;
            
            if (socketData < 0) {
                envoyerReponse(socketClient, "425 Cannot open data connection.");
                return;
            }
        }
        
        // Envoyer le fichier
        std::ifstream fichier(cheminFichier, std::ios::binary);
        if (!fichier) {
            close(socketData);
            envoyerReponse(socketClient, "550 Failed to open file.");
            return;
        }
        
        char bufferFichier[8192];
        while (!fichier.eof()) {
            fichier.read(bufferFichier, sizeof(bufferFichier));
            std::streamsize bytesLus = fichier.gcount();
            if (bytesLus > 0) {
                write(socketData, bufferFichier, bytesLus);
            }
        }
        
        fichier.close();
        close(socketData);
        
        envoyerReponse(socketClient, "226 Transfer complete.");
    } else if (nomCommande == "STOR") {
        // Uploader un fichier
        if (argument.empty()) {
            envoyerReponse(socketClient, "501 Paramètre requis.");
            return;
        }
        
        std::string cheminFichier = client->cheminCourant + "/" + argument;
        
        envoyerReponse(socketClient, "150 Ready for file transfer.");
        
        int socketData = -1;
        
        // Établir la connexion de données
        if (client->modeDonneesActif) {
            // Mode actif
            socketData = socket(AF_INET, SOCK_STREAM, 0);
            if (socketData < 0) {
                envoyerReponse(socketClient, "425 Cannot open data connection.");
                return;
            }
            
            struct sockaddr_in adresseData;
            memset(&adresseData, 0, sizeof(adresseData));
            adresseData.sin_family = AF_INET;
            adresseData.sin_port = htons(client->portDataActif);
            inet_pton(AF_INET, client->adresseDataActif.c_str(), &adresseData.sin_addr);
            
            if (connect(socketData, (struct sockaddr*)&adresseData, sizeof(adresseData)) < 0) {
                close(socketData);
                envoyerReponse(socketClient, "425 Cannot open data connection.");
                return;
            }
        } else {
            // Mode passif
            struct sockaddr_in adresseClientData;
            socklen_t tailleClientData = sizeof(adresseClientData);
            socketData = accept(client->socketDonnees, (struct sockaddr*)&adresseClientData, &tailleClientData);
            close(client->socketDonnees);
            client->socketDonnees = -1;
            
            if (socketData < 0) {
                envoyerReponse(socketClient, "415 Cannot open data connection.");
                return;
            }
        }
        
        std::ifstream fichier(cheminFichier, std::ios::binary);
        if (!fichier) {
            close(socketData);
            envoyerReponse(socketClient, "550 Failed to create file.");
            return;
        }
        
        char bufferFichier[8192];
        while (!fichier.eof()) {
            fichier.read(bufferFichier, sizeof(bufferFichier));
            std::streamsize bytesLus = fichier.gcount();
            if (bytesLus > 0) {
                write(socketData, bufferFichier, bytesLus);
            }
        }
        
        fichier.close();
        close(socketData);
        
        envoyerReponse(socketClient, "226 Transfer complete.");
    } else if (nomCommande == "HELP") {
        std::string aide;
        if (argument.empty()) {
            aide = "214 Commandes disponibles:\r\n"
                    "USER <username>: Spécifier l'utilisateur pour l'authentification\r\n"
                    "PASS <password>: Spécifier le mot de passe pour l'authentification\r\n"
                    "CWD <pathname>: Changer le répertoire de travail\r\n"
                    "CDUP: Remonter au répertoire parent\r\n"
                    "QUIT: Se déconnecter du serveur\r\n"
                    "DELE <pathname>: Supprimer un fichier sur le serveur\r\n"
                    "PWD: Afficher le répertoire de travail actuel\r\n"
                    "PASV: Activer le mode passif pour le transfert de données\r\n"
                    "PORT <host-port>: Activer le mode actif pour le transfert de données\r\n"
                    "HELP: Afficher cette aide\r\n"
                    "NOOP: Ne rien faire\r\n"
                    "RETR <pathname>: Télécharger un fichier du serveur vers le client\r\n"
                    "STOR <pathname>: Téléverser un fichier du client vers le serveur\r\n"
                    "LIST [pathname]: Lister les fichiers dans le répertoire courant\r\n";
        } else {
            std::string cmd = argument;
            for(char& c : cmd) {
                c = toupper(c);
            }
            if (cmd == "USER") {
                aide = "214 USER <username>: Spécifier l'utilisateur pour l'authentification.";
            } else if (cmd == "PASS") {
                aide = "214 PASS <password>: Spécifier le mot de passe pour l'authentification.";
            } else if (cmd == "CWD") {
                aide = "214 CWD <pathname>: Changer le répertoire de travail.";
            } else if (cmd == "CDUP") {
                aide = "214 CDUP: Remonter au répertoire parent.";
            } else if (cmd == "QUIT") {
                aide = "214 QUIT: Se déconnecter du serveur.";
            } else if (cmd == "DELE") {
                aide = "214 DELE <pathname>: Supprimer un fichier sur le serveur.";
            } else if (cmd == "PWD") {
                aide = "214 PWD: Afficher le répertoire de travail actuel.";
            } else if (cmd == "PASV") {
                aide = "214 PASV: Activer le mode passif pour le transfert de données.";
            } else if (cmd == "PORT") {
                aide = "214 PORT <host-port>: Activer le mode actif pour le transfert de données.";
            } else if (cmd == "HELP") {
                aide = "214 HELP [cmd]: Afficher l'aide.";
            } else if (cmd == "NOOP") {
                aide = "214 NOOP: Ne rien faire.";
            } else if (cmd == "RETR") {
                aide = "214 RETR <pathname>: Télécharger un fichier du serveur vers le client.";
            } else if (cmd == "STOR") {
                aide = "214 STOR <pathname>: Téléverser un fichier du client vers le serveur.";
            } else if (cmd == "LIST") {
                aide = "214 LIST [pathname]: Lister les fichiers dans le répertoire courant.";
            } else {
                aide = "502 Commande non implémentée.";
            }
        }
        envoyerReponse(socketClient, aide);
    } else if (nomCommande == "NOOP") {
        envoyerReponse(socketClient, "200 OK");
    }
}

void ServeurFTP::envoyerReponse(int socketClient, const std::string& reponse)
{
    std::string reponseComplete = reponse + "\r\n";
    write(socketClient, reponseComplete.c_str(), reponseComplete.length());
    std::cout << "Réponse envoyée au client " << socketClient << ": " << reponse << std::endl;
}