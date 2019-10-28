PROJET OS


le projet contient plusieurs fichiers:

-os.h/os.c : fichiers contient le coeur du systeme (gestion de m�moire,virtualisation,traitement des requ�tes,..)
-request_queue.h/request_queue.c : impl�mentation des deux files de requ�tes et de r�ponses.
-netio.h/netio.c : tous ce qui a rapport avec lecture/�criture r�seau (NetListener, NetResponder)
-targa_creator.h/targa_creator.c : tous ce qui a rapport avec la cr�ation des images, dessin de texte, st�ganographie.
-client.h/client.c : programme client
-main.c: driver du syst�me, initiation, impl�mentation des Pi,...
-loadbmp.h: biblioth�que pour charger font.bmp(image qui d�finit le font du texte � utiliser)
-stegano_extractor: programme qui effectue l'extraction des informations de tra�abilit� inject�es par st�ganographie.

pour compiler le projet:

-make all : compile le serveur
-make client: compile le client
-make stegano_extractor: compile stegano_extractor