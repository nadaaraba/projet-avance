/// \file log.c
/// \author Nada ARABA
/// \brief Fichier de log utilisé par rsa_tools.c et bezout.c (variable globale logfp)
///
/// rsa_common_header.h déclare "extern FILE *logfp;" mais personne ne la définissait :
/// sans ce fichier le programme ne se lie même pas. On la définit ici, UNE seule fois,
/// pour que tous les programmes de test (les nôtres et ceux des autres) l'utilisent.
#include "../rsa_common_header.h"
#include "phase1.h"

FILE *logfp = NULL;

void init_log(void){
  logfp = fopen("diypg.log", "w");
  if (logfp == NULL)      // impossible d'écrire le log : on n'arrête pas le programme pour ça
    logfp = stderr;
}

void close_log(void){
  if (logfp != NULL && logfp != stderr)
    fclose(logfp);
  logfp = NULL;
}
