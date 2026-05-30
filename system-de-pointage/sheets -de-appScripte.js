// =========================================================================
// FUNCTION PRINCIPALE : RECEPTION DES SCANS DE L'ESP32 (ARRIVÉE / DÉPART)
//l'integratio ce fait via l'onglet appscripte de google sheets 
// =========================================================================
function doPost(e) {
  var data = JSON.parse(e.postData.contents);
  var uid = data.uid;
  
  var ss = SpreadsheetApp.getActiveSpreadsheet();
  var sheetLog = ss.getSheetByName("Pointage");
  
  var maintenant = new Date();
  var dateJour = maintenant.toLocaleDateString("fr-FR");
  var heureActuelle = maintenant.toLocaleTimeString("fr-FR");

  // Liste exacte des feuilles de classes correspondant aux onglets du Sheets
  var classes = ["6eme", "5eme", "4eme", "3eme", "2end", "1er", "TD"];
  var eleveTrouve = null;
  var nomComplet = "";
  var classeDeLEleve = "";
  var sheetClasseCible = null;
  var ligneEleveCible = -1;
  var versementInfo = "";

  // 1. RECHERCHE DE L'ÉLÈVE DANS TOUTES LÈS CLASSES
  for (var i = 0; i < classes.length; i++) {
    var sheet = ss.getSheetByName(classes[i]);
    if (sheet) {
      var valeurs = sheet.getDataRange().getValues();
      for (var j = 1; j < valeurs.length; j++) {
        if (valeurs[j][0].toString().trim().toUpperCase() == uid.toString().trim().toUpperCase()) {
          nomComplet = valeurs[j][1] + " " + valeurs[j][2]; // Nom + Prénom
          classeDeLEleve = classes[i];
          sheetClasseCible = sheet;
          ligneEleveCible = j + 1;
          versementInfo = valeurs[j][4]; // Colonne E (Versements)
          eleveTrouve = valeurs[j];
          break;
        }
      }
    }
    if (eleveTrouve) break;
  }

  // Si le badge n'existe dans aucune classe
  if (!eleveTrouve) {
    return ContentService.createTextOutput(JSON.stringify({"status":"Inconnu"})).setMimeType(ContentService.MimeType.JSON);
  }

  // 2. VÉRIFICATION DE L'HISTORIQUE DE LA JOURNÉE POUR CET UTILISATEUR
  var logValues = sheetLog.getDataRange().getValues();
  var logRow = -1;
  
  for (var k = logValues.length - 1; k >= 0; k--) {
    if (logValues[k][0] == uid && logValues[k][3] == dateJour && logValues[k][5] == "") {
      logRow = k + 1; // Ligne du scan d'arrivée sans heure de départ
      break;
    }
  }

  // 3. EXECUTION DE LA LOGIQUE DE POINTAGE
  if (logRow == -1) {
    // ---- ENREGISTREMENT DE L'ARRIVÉE ----
    sheetLog.appendRow([uid, nomComplet, classeDeLEleve, dateJour, heureActuelle, "", "Présent"]);
    
    // Mise à jour visuelle de la feuille de classe
    sheetClasseCible.getRange(ligneEleveCible, 4).setValue("Présent"); // Colonne D
    sheetClasseCible.getRange(ligneEleveCible, 1, 1, 6).setBackground("#D4EDDA"); // Ligne en Vert clair
    
    return ContentService.createTextOutput(JSON.stringify({
      "status": "Arrivee", 
      "nom": nomComplet, 
      "info1": "Classe: " + classeDeLEleve, 
      "info2": "Scol: " + versementInfo
    })).setMimeType(ContentService.MimeType.JSON);
    
  } else {
    // ---- ENREGISTREMENT DE L'RETOUR / DÉPART ----
    var heureArriveeArr = logValues[logRow-1][4].split(":");
    var dateArrivee = new Date();
    dateArrivee.setHours(heureArriveeArr[0], heureArriveeArr[1], heureArriveeArr[2]);
    
    // Sécurité Anti-double scan (Seuil fixé à 2 minutes pour vos tests)
    if ((maintenant - dateArrivee) / 60000 < 2) {
      return ContentService.createTextOutput(JSON.stringify({"status":"Doublon", "nom": nomComplet})).setMimeType(ContentService.MimeType.JSON);
    }
    
    // Validation de la sortie dans le journal d'archive
    sheetLog.getRange(logRow, 6).setValue(heureActuelle);
    return ContentService.createTextOutput(JSON.stringify({"status": "Sortie", "nom": nomComplet})).setMimeType(ContentService.MimeType.JSON);
  }
}

// =========================================================================
// FUNCTION COMPLEMENTAIRE : TRAITEMENT ET ARCHIVAGE AUTOMATIQUE DES ABSENTS
// =========================================================================
function genererArchivesAbsences() {
  var ss = SpreadsheetApp.getActiveSpreadsheet();
  var sheetLog = ss.getSheetByName("Pointage");
  var classes = ["6eme", "5eme", "4eme", "3eme", "2end", "1er", "TD"];
  
  var dateJour = new Date().toLocaleDateString("fr-FR");
  
  for (var i = 0; i < classes.length; i++) {
    var sheet = ss.getSheetByName(classes[i]);
    if (sheet) {
      var valeurs = sheet.getDataRange().getValues();
      
      for (var j = 1; j < valeurs.length; j++) {
        var uid = valeurs[j][0];
        var nomComplet = valeurs[j][1] + " " + valeurs[j][2];
        var statutActuel = valeurs[j][3]; // Colonne D (Statut du jour)
        
        // Si la colonne statut est restée vide à l'heure du contrôle (Ex: 09h00)
        if (statutActuel == "" && uid != "") {
          // 1. Inscription définitive de l'absence dans l'archive Pointage
          sheetLog.appendRow([uid, nomComplet, classes[i], dateJour, "--", "--", "Absent"]);
          
          // 2. Marquage en rouge de l'élève absent dans sa classe
          sheet.getRange(j + 1, 4).setValue("Absent");
          sheet.getRange(j + 1, 1, 1, 6).setBackground("#F8D7DA"); // Rouge clair
        }
      }
    }
  }
}

// =========================================================================
// FUNCTION DE REINITIALISATION AUTOMATIQUE (À DÉCLENCHER CHAQUE MINUIT)
// =========================================================================
function reinitialisationQuotidienne() {
  var ss = SpreadsheetApp.getActiveSpreadsheet();
  var classes = ["6eme", "5eme", "4eme", "3eme", "2end", "1er", "TD"];
  
  for (var i = 0; i < classes.length; i++) {
    var sheet = ss.getSheetByName(classes[i]);
    if (sheet) {
      var derniereLigne = sheet.getLastRow();
      if (derniereLigne > 1) {
        // Efface le contenu de la colonne D (Statut) pour le nouveau jour
        sheet.getRange(2, 4, derniereLigne - 1, 1).setValue("");
        // Réinitialise la couleur blanche d'origine sur tout le tableau
        sheet.getRange(2, 1, derniereLigne - 1, 6).setBackground("#FFFFFF");
      }
    }
  }
}