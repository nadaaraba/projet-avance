/// \file main_diypg.c
/// \author Nada ARABA
/// \brief Programme principal de DIYPG (sujet, phase 3 : « un main qui appellera l'interprete »).
///
///     diypg            mode interactif (invite "diypg> ")
///     diypg script     rejoue les commandes du fichier, en les recopiant
#include <stdio.h>
#include <stdlib.h>
#include "interprete.h"
#include "../phase1_nada/phase1/phase1.h"   /* init_log, close_log */
#include "../phase1_loann/base64.h"         /* base64_cleanup */
#include "../console_utf8.h"

int main(int argc, char **argv) {
    interprete_t it;
    FILE *entree = stdin;
    if (argc > 2) {
        fprintf(stderr, "Usage : %s [script]\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (argc == 2 && (entree = fopen(argv[1], "r")) == NULL) {
        fprintf(stderr, "Impossible d'ouvrir le script %s\n", argv[1]);
        return EXIT_FAILURE;
    }
    initialiser_console_utf8();
    init_log();
    interprete_initialiser(&it);
    if (argc == 1) printf("DIYPG : tapez help pour l'aide, quit pour sortir.\n");
    int erreurs = interprete_boucle(&it, entree, stdout, argc == 2, argc == 1);
    if (argc == 2) printf("Script termine : %d commande(s) en erreur.\n", erreurs);
    interprete_liberer(&it);
    base64_cleanup();
    close_log();
    if (entree != stdin) fclose(entree);
    return fflush(stdout) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
