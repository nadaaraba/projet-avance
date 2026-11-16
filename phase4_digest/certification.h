/// \file certification.h
/// \author Nada ARABA
/// \brief Phase 4 : requetes a l'autorite de certification (interprete.pdf : certify <id> et revoke <id>,
///        « envoie une requete a l'autorite de certification (ecriture dans un fichier) »).
///
/// Chaque requete est une ligne ajoutee a la fin du fichier de requetes, que la phase 5 lira comme
/// une file de transactions :
///     <code> <date> <keyid> <crypt|sign> <E hex> <N hex> <digest>
/// code : NCK / NSK (declaration d'une clef de chiffrement / de signature), RCK / RSK (revocation),
/// comme les transactions de la phase 5. date : secondes depuis 1970 (time_t).
/// digest : SHA-256 en hexadecimal de la ligne sans ce dernier champ (ni l'espace qui le precede).
/// Seule la clef publique est ecrite.
#ifndef CERTIFICATION_H
#define CERTIFICATION_H

#include <time.h>
#include "../phase3_nada/interpreteur.h"

#define CERTIF_LIGNE_MAX 256

typedef enum { CERTIF_DECLARATION, CERTIF_REVOCATION } certif_action_t;

/* "NCK", "NSK", "RCK" ou "RSK". */
const char *certif_code(certif_action_t action, type_clef_t type);

/*
 * Ajoute la requete pour la clef <keyid> de l'utilisateur (pas une clef de contact).
 * Si code n'est pas NULL, il recoit le code de la requete.
 * Retourne CMD_SUCCES, CMD_ERREUR_ID_INCONNU, RSA_ERREUR_PARAMETRE ou RSA_ERREUR_FICHIER.
 */
int certif_requete(const table_clefs_t *table, const char *keyid, certif_action_t action,
                   const char *fichier, time_t date, const char **code);

/* 1 si la ligne (avec ou sans '\n' final) a le bon format et un digest correct, 0 sinon. */
int certif_verifier_ligne(const char *ligne);

#endif
