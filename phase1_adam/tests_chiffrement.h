#ifndef TESTS_CHIFFREMENT_PHASE1_H
#define TESTS_CHIFFREMENT_PHASE1_H

/*
 * A appeler depuis le main de test commun, avec un dossier temporaire existant.
 * Retourne 0 si tous les tests passent, 1 sinon.
 */
int tester_chiffrement_adam(const char *dossier_temporaire);

#endif
