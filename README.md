# Client TFTP

La commande "gettftp" permet de télécharger un fichier depuis le serveur sur un ou plusieurs paquets.
En revanche, la commande "puttftp" permet d'envoyer un seul paquet sur le serveur. En effet, le serveur n'envoit pas l'acquitement du premier paquet de donnée et n'est donc jamais prêt à recevoir le second. Je n'ai pas réussi à résoudre ce problème. Ci dessous deux captures des requêtes d'écriture et de lecture.

## Visualisation de la requête RRQ sur Wireshark

Obtenue grâce à la commande :

> $ ./gettfp 127.0.0.1:1069 ones1024

![](captures_wireshark/RRQ.png)

## Visualisation de la requête WRQ sur Wireshark

Obtenue grâce à la commande :

> $ ./puttfp 127.0.0.1:1069 ones1024

![](captures_wireshark/WRQ.png)
