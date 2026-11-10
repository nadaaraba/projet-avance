#include "contacts_interne.h"
#include <errno.h>
#include <inttypes.h>

static int lire_ligne(FILE *entree, char *texte, size_t capacite)
{
    size_t taille = 0;
    int c, invalide = 0;
    while ((c = fgetc(entree)) != EOF && c != '\n') {
        if (c == '\r') {
            c = fgetc(entree);
            if (c == '\n' || c == EOF) break;
            invalide = 1;
        }
        if (c == '\0') invalide = 1;
        if (taille + 1 < capacite) texte[taille++] = (char)c;
        else invalide = 1;
    }
    texte[taille] = '\0';
    if (ferror(entree)) return CONTACTS_ERREUR_FICHIER;
    if (invalide || (c == EOF && taille == 0)) return CONTACTS_ERREUR_SAISIE;
    return 0;
}

static int demander(FILE *entree, FILE *sortie, const char *question,
                    char *texte, size_t capacite)
{
    if (fputs(question, sortie) == EOF || fflush(sortie) != 0)
        return CONTACTS_ERREUR_FICHIER;
    return lire_ligne(entree, texte, capacite);
}

static int ajouter_contact(annuaire_contacts_t *annuaire, const char *id,
                           FILE *entree, FILE *sortie)
{
    contact_infos_t infos;
    int resultat;
    if (!contacts_identifiant_valide(id)) return CONTACTS_ERREUR_IDENTIFIANT;
    if (contacts_trouver(annuaire, id) != NULL) return CONTACTS_ERREUR_DOUBLON;
    if ((resultat = demander(entree, sortie, "Nom : ", infos.nom, sizeof infos.nom)) != 0 ||
        (resultat = demander(entree, sortie, "Prénom : ", infos.prenom, sizeof infos.prenom)) != 0 ||
        (resultat = demander(entree, sortie, "Commentaire : ", infos.commentaire,
                              sizeof infos.commentaire)) != 0) return resultat;
    return contacts_ajouter(annuaire, id, infos.nom, infos.prenom, infos.commentaire);
}

static int modifier_contact(annuaire_contacts_t *annuaire, const char *id,
                            FILE *entree, FILE *sortie)
{
    const contact_infos_t *contact = contacts_trouver(annuaire, id);
    contact_infos_t infos;
    char choix[8];
    int resultat;
    if (contact == NULL) return CONTACTS_ERREUR_INTROUVABLE;
    infos = *contact;
    resultat = contacts_afficher(annuaire, id, NULL, sortie);
    if (resultat != CONTACTS_SUCCES) return resultat;
    for (;;) {
        if (fputs("1. Nom\n2. Prénom\n3. Commentaire\n0. Enregistrer\n", sortie) == EOF)
            return CONTACTS_ERREUR_FICHIER;
        resultat = demander(entree, sortie, "Choix : ", choix, sizeof choix);
        if (resultat != 0) return resultat;
        if (strcmp(choix, "0") == 0)
            return contacts_modifier(annuaire, id, infos.nom, infos.prenom, infos.commentaire);
        if (strcmp(choix, "1") == 0)
            resultat = demander(entree, sortie, "Nom : ", infos.nom, sizeof infos.nom);
        else if (strcmp(choix, "2") == 0)
            resultat = demander(entree, sortie, "Prénom : ", infos.prenom, sizeof infos.prenom);
        else if (strcmp(choix, "3") == 0)
            resultat = demander(entree, sortie, "Commentaire : ", infos.commentaire,
                                  sizeof infos.commentaire);
        else
            return CONTACTS_ERREUR_SAISIE;
        if (resultat != 0) return resultat;
    }
}

static int lire_hexadecimal(const char *texte, uint64_t *valeur)
{
    char *fin;
    uintmax_t nombre;
    if (texte[0] == '\0' || texte[0] == '-' || texte[0] == '+' ||
        texte[0] == ' ' || texte[0] == '\t') return CONTACTS_ERREUR_SAISIE;
    errno = 0;
    nombre = strtoumax(texte, &fin, 16);
    if (errno == ERANGE || fin == texte || *fin != '\0' || nombre > UINT64_MAX)
        return CONTACTS_ERREUR_SAISIE;
    *valeur = (uint64_t)nombre;
    return 0;
}

static int ajouter_cle(annuaire_contacts_t *annuaire, const char *contact,
                       FILE *entree, FILE *sortie)
{
    char id[NAME_MAX_SIZE + 1], type[16], texte[32];
    rsaKey_t cle;
    contact_type_cle_t type_cle;
    contact_t *selection = NULL;
    int resultat;
    resultat = contacts_chercher_id_ou_nom(annuaire, contact, &selection);
    if (resultat != 0) return resultat;
    if ((resultat = demander(entree, sortie, "Identifiant de la clef : ", id, sizeof id)) != 0 ||
        (resultat = demander(entree, sortie, "Type (crypt/sign) : ", type, sizeof type)) != 0)
        return resultat;
    if (strcmp(type, "crypt") == 0) type_cle = CONTACT_CRYPT;
    else if (strcmp(type, "sign") == 0) type_cle = CONTACT_SIGN;
    else return CONTACTS_ERREUR_TYPE;
    if ((resultat = demander(entree, sortie, "Exposant public E (hex) : ", texte, sizeof texte)) != 0 ||
        (resultat = lire_hexadecimal(texte, &cle.E)) != 0 ||
        (resultat = demander(entree, sortie, "Module N (hex) : ", texte, sizeof texte)) != 0 ||
        (resultat = lire_hexadecimal(texte, &cle.N)) != 0) return resultat;
    return contacts_ajouter_cle(annuaire, contact, id, type_cle, &cle);
}

int contacts_executer(annuaire_contacts_t *annuaire, int argc_total,
                     const char *const argv_total[], FILE *entree, FILE *sortie)
{
    if (annuaire == NULL || argc_total < 1 || argv_total == NULL ||
        entree == NULL || sortie == NULL)
        return CONTACTS_ERREUR_PARAMETRE;
    for (int i = 0; i < argc_total; ++i)
        if (argv_total[i] == NULL) return CONTACTS_ERREUR_PARAMETRE;
    /* argv_total[0] est le nom de la commande ; la suite traite seulement ses arguments. */
    const char *commande = argv_total[0];
    int argc = argc_total - 1;
    const char *const *argv = argv_total + 1;
    if (strcmp(commande, "listcontacts") == 0) {
        if (argc > 2) return CONTACTS_ERREUR_PARAMETRE;
        return contacts_afficher(annuaire, argc > 0 ? argv[0] : NULL,
                                 argc > 1 ? argv[1] : NULL, sortie);
    }
    if (strcmp(commande, "addcontact") == 0)
        return argc == 1 ? ajouter_contact(annuaire, argv[0], entree, sortie) :
                           CONTACTS_ERREUR_PARAMETRE;
    if (strcmp(commande, "modifycontact") == 0)
        return argc == 1 ? modifier_contact(annuaire, argv[0], entree, sortie) :
                           CONTACTS_ERREUR_PARAMETRE;
    if (strcmp(commande, "addkeys") == 0)
        return argc == 1 ? ajouter_cle(annuaire, argv[0], entree, sortie) :
                           CONTACTS_ERREUR_PARAMETRE;
    if (strcmp(commande, "rmcontact") == 0)
        return argc == 1 ? contacts_supprimer(annuaire, argv[0]) :
                           CONTACTS_ERREUR_PARAMETRE;
    if (strcmp(commande, "rmkeys") == 0 && argc > 0 && strchr(argv[0], '/') != NULL)
        return argc == 1 ? contacts_supprimer_cle(annuaire, argv[0]) :
                           CONTACTS_ERREUR_PARAMETRE;
    return CONTACTS_COMMANDE_INCONNUE;
}
