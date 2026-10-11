## Phase 1.0 : étude de MAX_PRIME et bilan des tests (partie de Nada)

### Valeur de MAX_PRIME
- Valeur par défaut (rsa_common_header.h) : **10000**. C'est la valeur retenue pour les tests courants.
- Valeur maximale **sûre** : **65536**. Avec deux nombres premiers p et q inférieurs à 65536, N = p.q est
  inférieur à 2^32. Or `puissance_mod_n` calcule `a*a` avant le `% n` : ça ne tient dans un `uint64_t`
  que si a < 2^32, c'est-à-dire tant que N < 2^32 (4 294 967 296).
- Au-delà, des clefs dont le module N dépasse 2^32 apparaissent et le déchiffrement devient faux.

### Mesures (programme `test_maxprime`, 2000 paires de clefs par ligne, chaque paire testée sur les 256 valeurs d'un octet)

| MAX_PRIME | N maximal observé | paires avec N >= 2^32 | paires en échec |
|---|---|---|---|
| 1 000 | 980 051 | 0 | 0 |
| 10 000 | 93 881 503 | 0 | 0 |
| 30 000 | 871 610 407 | 0 | 0 |
| 60 000 | 3 480 925 471 | 0 | 0 | 
| 65 536 | 4 058 524 373 | 0 | 0 |
| 70 000 | 4 676 538 067 | 14 | 14 |
| 80 000 | 6 209 139 191 | 120 | 120 |
| 100 000 | 9 619 535 129 | 362 | 362 |
| 200 000 | 38 660 197 529 | 1 330 | 1 330 |
| 1 000 000 | 986 971 496 639 | 1 942 | 1 942 |
| 5 000 000 | 24 882 081 958 121 | 1 994 | 1 994 |

**Constat : le nombre de paires en échec est exactement le nombre de paires avec N >= 2^32.**
Aucune paire avec N < 2^32 n'échoue, et toute paire avec N >= 2^32 échoue : le seuil de
dépassement de capacité est donc N = 2^32, ce qui correspond à MAX_PRIME = 65536 environ.
(Les nombres exacts varient un peu d'un lancement à l'autre car les tirages sont aléatoires.)
C'est ce dépassement qui justifie la phase 2 (GMP).

### Anomalies trouvées dans le code fourni et corrigées
1. `logfp` (déclaré `extern` dans le header) n'était défini nulle part : ajout de `log.c` (`init_log()`).
2. `assert(num1 != num2)` dans `genKeysRabin` faisait planter le programme dès que les deux tirages
   donnaient le même premier : remplacé par un nouveau tirage.
3. Le test de Rabin n'utilisait que la base 2 : 2047 (= 23 x 89), 3277, 4033, 4681 et 8321 sont déclarés
   premiers à tort, ce qui produisait des clefs fausses. Nouveau `rabin_multi` (bases 2, 3, 5, 7).
4. `genereUintRabin` pouvait renvoyer un nombre >= max sans être premier (exemple : 10001 = 73 x 137) :
   on recommence le tirage dans ce cas.
5. `rand()` sans `srand()` donne les mêmes clefs à chaque lancement : les mains de test appellent
   `srand(time(NULL))`.
6. Formats d'affichage `%llu` incorrects pour des `uint64_t` / `int64_t` : corrigés.

Après ces corrections : 0 paire fausse sur 4 x 1000 paires testées avec MAX_PRIME = 10000
(avant : environ 0,8 % de paires fausses, et un arrêt du programme par `assert`).
