===================================================================
   Mon projet de Pointage RFID pour le Lycee d'Excellence 
===================================================================

C'est le dépôt de mon système de gestion des présences en IoT. 
pour automatiser le pointage des élèves avec des badges RFID 
sans que les surveillants galèrent avec les fiches papier tous les matins.

 100% autonome :
- Quand le réseau marche : il envoie direct les scans sur un Google Sheets 
  en Wi-Fi (nom, classe, heure d'arrivée ou de sortie).
- Si le Wi-Fi coupe (ce qui arrive souvent en pratique) : l'ESP32 ne plante pas. 
  Il sauvegarde direct les codes des badges sur une carte Micro SD dans le boîtier.
- Il a aussi un serveur web intégré. On se connecte sur l'IP de la carte 
  et on voit le dashboard des connexions en local sur son téléphone ou PC.

Le matériel que j'ai utilisé pour le montage :
* ESP32 (WROOM-32)
* Lecteur RFID RC522 (câblé en SPI)
* Écran LCD 16x4 (avec module I2C pour économiser les broches)
* Lecteur de carte Micro SD (SPI aussi)
* Un buzzer + 2 LED (Verte pour accès OK / Rouge pour doublon ou inconnu)

Le code gère l'anti-triche pour éviter qu'un élève badge deux fois de suite.

