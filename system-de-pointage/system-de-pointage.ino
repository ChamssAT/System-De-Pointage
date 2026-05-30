// =========================================================================
                         //بِسْمِ ٱللَّٰهِ ٱلرَّحْمَٰنِ ٱلرَّحِيمِ
// =========================================================================
#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>


#define SS_PIN  5   
#define RST_PIN 4  
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Config de  LCD 16x4 
LiquidCrystal_I2C lcd(0x27, 16, 4);

const int pinLedVerte = 12;
const int pinLedRouge = 14;
const int pinBuzzer   = 13;


const char* ssid = "A";
const char* password = "90515204";
const char* googleScriptUrl = "https://script.google.com/macros/s/AKfycbxRkWvYJSh50Tn4t0FrDQ9zW7V9fwXiBCugGAiZ8sBcIZwBOCG__FcXHXaenSR1R37L/exec";

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  
  pinMode(pinLedVerte, OUTPUT);
  pinMode(pinLedRouge, OUTPUT);
  pinMode(pinBuzzer, OUTPUT);
  digitalWrite(pinLedVerte, LOW);
  digitalWrite(pinLedRouge, LOW);
  digitalWrite(pinBuzzer, LOW);

 
  Wire.begin(25, 26); 
  lcd.init();
  lcd.backlight();
  
  
  lcd.setCursor(0, 0); lcd.print("   LYCEE EXCELLENCE  ");
  lcd.setCursor(0, 1); lcd.print(" SYSTEME POINTAGE ");
  lcd.setCursor(0, 2); lcd.print("Init du materiel..");

  // Initialisation du protocole SPI et du lecteur RFID (Utilise MOSI:23, MISO:19, SCK:18)
  SPI.begin();
  mfrc522.PCD_Init();

  // Connexion au réseau Wi-Fi
  WiFi.begin(ssid, password);
  lcd.setCursor(0, 3); lcd.print("Connexion WiFi... ");
  Serial.println("Tentative de connexion au Wi-Fi...");
  
  // Attente active de la connexion avec affichage de diagnostic
  int tentatives = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    tentatives++;
    
    // Si au bout de 20 essais (10 secondes) ça ne marche pas, diagnostic sur le moniteur
    if(tentatives > 20) {
      Serial.println("\nConnexion longue... Verifie que ton reseau est bien en 2.4GHz");
      tentatives = 0;
    }
  }
  
  Serial.println("\nWiFi connecte avec succes !");
  Serial.print("Adresse IP de l'ESP32 : ");
  Serial.println(WiFi.localIP());
  
  // Affichage du système prêt à l'emploi
  reinitialiserAffichage();
}

void loop() {
  // Détection active d'un badge RFID devant le lecteur
  if ( ! mfrc522.PICC_IsNewCardPresent() || ! mfrc522.PICC_ReadCardSerial() ) {
    delay(5);
    return;
  }
  
  // Traduction de l'UID du badge lu en chaîne de caractères Hexadécimale
  String uidString = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uidString += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
    uidString += String(mfrc522.uid.uidByte[i], HEX);
  }
  uidString.toUpperCase();
  
  Serial.print("Badge detecte ! Code UID : ");
  Serial.println(uidString);
  
  // Bip mécanique immédiat confirmant la capture physique du badge
  digitalWrite(pinBuzzer, HIGH); delay(1000); digitalWrite(pinBuzzer, LOW);

  // Traitement visuel temporaire sur l'écran
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Badge detecte !");
  lcd.setCursor(0, 1); lcd.print("Analyse en cours");

  // Envoi de la requête réseau si la connexion Wi-Fi est opérationnelle
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS); // Gère la redirection Google 302
    http.begin(googleScriptUrl);
    http.addHeader("Content-Type", "application/json");
    
    // Encapsulation de l'UID au format JSON
    StaticJsonDocument<200> doc;
    doc["uid"] = uidString;
    String requestBody;
    serializeJson(doc, requestBody);
    
    int httpResponseCode = http.POST(requestBody);
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      StaticJsonDocument<500> jsonReponse;
      DeserializationError error = deserializeJson(jsonReponse, response);
      
      if (!error) {
        String status = jsonReponse["status"];
        String nom = jsonReponse["nom"];
        
        lcd.clear();
        
        if (status == "Arrivee") {
          String infoClasse = jsonReponse["info1"];
          String infoScolarite = jsonReponse["info2"];
          
          // Signal de succès : LED Verte stable et double bip joyeux
          digitalWrite(pinLedVerte, HIGH);
          digitalWrite(pinBuzzer, HIGH); delay(2000); digitalWrite(pinBuzzer, LOW); delay(2000);
          digitalWrite(pinBuzzer, HIGH); delay(2000); digitalWrite(pinBuzzer, LOW);
          
          lcd.setCursor(0, 0); lcd.print("ARRIVEE VALIDEE");
          lcd.setCursor(0, 1); lcd.print(nom.substring(0, 16));
          lcd.setCursor(0, 2); lcd.print(infoClasse);
          lcd.setCursor(0, 3); lcd.print(infoScolarite);
          
        } else if (status == "Sortie") {
          // Signal de Sortie : LED Verte et un bip moyen
          digitalWrite(pinLedVerte, HIGH);
          digitalWrite(pinBuzzer, HIGH); delay(2000); digitalWrite(pinBuzzer, LOW);
          
          lcd.setCursor(0, 0); lcd.print("SORTIE VALIDEE");
          lcd.setCursor(0, 1); lcd.print(nom.substring(0, 16));
          lcd.setCursor(0, 2); lcd.print("Bon retour chez");
          lcd.setCursor(0, 3); lcd.print("vous !");
          
        } else if (status == "Doublon") {
          // Signal d'avertissement : LED Rouge et un bip long de refus
          digitalWrite(pinLedRouge, HIGH);
          digitalWrite(pinBuzzer, HIGH); delay(1000); digitalWrite(pinBuzzer, LOW);
          
          lcd.setCursor(0, 0); lcd.print("DEJA ENREGISTRE");
          lcd.setCursor(0, 1); lcd.print(nom.substring(0, 16));
          lcd.setCursor(0, 2); lcd.print("Veuillez circuler");
          
        } else { // Cas "Inconnu" (Le badge n'est pas répertorié)
          digitalWrite(pinLedRouge, HIGH);
          digitalWrite(pinBuzzer, HIGH); delay(800); digitalWrite(pinBuzzer, LOW);
          
          lcd.setCursor(0, 0); lcd.print("ACCES REFUSE !");
          lcd.setCursor(0, 1); lcd.print("Badge Inconnu");
          lcd.setCursor(0, 2); lcd.print("Contactez admin");
        }
      }
    } else {
      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("Erreur HTTP !");
      lcd.setCursor(0, 1); lcd.print("Code: " + String(httpResponseCode));
    }
    http.end();
  } else {
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("Erreur Reseau");
    lcd.setCursor(0, 1); lcd.print("WiFi Deconnecte");
  }
  
  // Temporisation de maintien d'affichage des données à l'écran (4 secondes)
  delay(1000);
  
  // Extinction des LED de notification et retour à l'état de veille
  digitalWrite(pinLedVerte, LOW);
  digitalWrite(pinLedRouge, LOW);
  reinitialiserAffichage();
}

void reinitialiserAffichage() {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("  SOCIETE READY  ");
  lcd.setCursor(0, 1); lcd.print("  SCANNER BADGE  ");
  lcd.setCursor(0, 2); lcd.print("   S.V.P POINTE  ");
}
