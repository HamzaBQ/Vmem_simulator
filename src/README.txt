PROJET OS


le projet contient plusieurs fichiers:

-os.h/os.c : fichiers contient le coeur du systeme (gestion de mémoire,virtualisation,traitement des requêtes,..)
-request_queue.h/request_queue.c : implémentation des deux files de requêtes et de réponses.
-netio.h/netio.c : tous ce qui a rapport avec lecture/écriture réseau (NetListener, NetResponder)
-targa_creator.h/targa_creator.c : tous ce qui a rapport avec la création des images, dessin de texte, stéganographie.
-client.h/client.c : programme client
-main.c: driver du système, initiation, implémentation des Pi,...
-loadbmp.h: bibliothèque pour charger font.bmp(image qui définit le font du texte à utiliser)
-stegano_extractor: programme qui effectue l'extraction des informations de traçabilité injectées par stéganographie.

pour compiler le projet:

-make all : compile le serveur
-make client: compile le client
-make stegano_extractor: compile stegano_extractor