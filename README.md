MEGAPHONE
=========

## Sommaire

1. [Introduction et informations](README.md#introduction-et-informations)
2. [Compilation et Exection du projet](README.md#compilation-et-execution-du-projet)
3. [Fonctionalite](README.md#fonctionnalites)

----------------------------------------------------------------------

## Introduction et informations

Le but de ce projet est d’implémenter un serveur et un client pour le protocole *Mégaphone*.
Des utilisateurs se connectent à un serveur avec un client et peuvent ensuite suivre des fils de discussions, en créer de nouveaux ou enrichir des fils existants en postant un nouveau message (billet), s’abonner à un fil pour recevoir des notifications.

**Informations généraux**

- Le fichier authors : [AUTHORS.md](AUTHORS.md)

----------------------------------------------------------------------

## Compilation et Execution du projet

**Compilation**

Pour compiler le projet, il suffit d'entrer la commande suivante :

```
make [all]
```

Pour compiler le client seul :

```
make client
```

Pour compiler le serveur seul :

```
make server
```

Pour supprimer les fichiers .o et l'exécutables :

```
make clean
```

Pour tester valgrind côté client :

```
make client_memory
```

Pour tester valgrind côté serveur :

```
make server_memory
```

Pour afficher l'aide des option de make

```
make help
```

**Execution**

Pour exécuter le client :

```
./bin/client [-i _nom_de_la_machine_] [-p _port_]
```

Pour exécuter le serveur :

```
./bin/server [-t _port_tcp_] [-u _port_udp]
```

----------------------------------------------------------------------

## Fonctionnalites

**Inscription**
**Nouveau Post**
**Derniers Post**
**Envoyer un fichier**
**Telecharger un fichier**
**Notification**
**Extention**
---------------------------------------------------------------------
