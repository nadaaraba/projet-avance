/* Teste les commandes V2 directement sur l'annuaire, sans l'interprete. */
#include "../contacts.h"
#include "../../console_utf8.h"

int main(int argc, char **argv)
{
    annuaire_contacts_t annuaire = {NULL};
    FILE *entree;
    char ligne[256];
    int resultat = EXIT_SUCCESS;
    initialiser_console_utf8();
    if (argc != 2 || (entree = fopen(argv[1], "rb")) == NULL) return EXIT_FAILURE;
    while (fgets(ligne, sizeof ligne, entree) != NULL) {
        const char *arguments[8];
        size_t nombre = 0;
        char *mot;
        if (strchr(ligne, '\n') == NULL && !feof(entree)) { resultat = EXIT_FAILURE; break; }
        for (mot = strtok(ligne, " \t\r\n"); mot != NULL; mot = strtok(NULL, " \t\r\n")) {
            if (nombre == 8) { resultat = EXIT_FAILURE; break; }
            arguments[nombre++] = mot;
        }
        if (resultat != EXIT_SUCCESS) break;
        if (nombre == 0) continue;
        if (strcmp(arguments[0], "quit") == 0) break;
        int code = contacts_executer(&annuaire, (int)nombre, arguments, entree, stdout);
        if (code != 0) {
            fprintf(stderr, "%s : %s\n", arguments[0], contacts_message_erreur(code));
            resultat = EXIT_FAILURE;
            break;
        }
    }
    if (ferror(entree)) resultat = EXIT_FAILURE;
    if (fclose(entree) != 0) resultat = EXIT_FAILURE;
    printf("\nScenario contacts : %zu contact(s) restant(s).\n", contacts_nombre(&annuaire));
    contacts_liberer(&annuaire);
    if (fflush(stdout) != 0) resultat = EXIT_FAILURE;
    return resultat;
}
