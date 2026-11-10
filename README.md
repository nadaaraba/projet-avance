# DIYPG — phases 1 et 2

Version commune des parties de Nada, Adam, Loann et Malek. Le point de départ est le commit `3772eaba` de la branche `phase1`.

## Récupérer le projet

Depuis GitHub, cloner la branche du groupe ou télécharger son ZIP. Après extraction, ouvrir un terminal dans `projet-avance`, au même niveau que ce README. Aucun chemin n'est à modifier dans les sources.

```sh
git clone --branch phase1-commune https://github.com/nadaaraba/projet-avance.git
cd projet-avance
```

La branche `phase1-commune` contient la version commune proposée au groupe. Son intégration à `phase1` se fait après relecture. La phase 2 est développée sur la branche `phase2`, partie de `phase1-commune` (voir [Phase 2 — RSA avec GMP](#phase-2--rsa-avec-gmp)).

## Compiler et tester

Le lanceur utilise **Python 3.9 ou plus**, un compilateur **GCC ou Clang** et, depuis la phase 2, la bibliothèque **GMP**. Python sert seulement à compiler et lancer les tests ; le projet reste en C. Aucun module Python supplémentaire n'est nécessaire. Make et les commandes Unix sont facultatifs avec ce lanceur.

### Windows

Installer [Python 3](https://www.python.org/downloads/) avec son lanceur `py`, puis télécharger [w64devkit](https://github.com/skeeto/w64devkit/releases). Extraire les outils de façon à avoir `projet-avance/w64devkit/bin/gcc.exe`. Ce dossier est détecté automatiquement et ignoré par Git. Un GCC déjà présent dans le PATH convient aussi. Pour GMP, voir [Installer GMP](#installer-gmp).

Dans PowerShell :

```powershell
.\tester.cmd
```

### Linux

Sur Ubuntu ou Debian, installer les outils une fois :

```sh
sudo apt update
sudo apt install build-essential python3 libgmp-dev
```

Puis lancer :

```sh
python3 tester.py
```

### macOS

Installer les outils de compilation Apple avec `xcode-select --install`, puis [Python 3](https://www.python.org/downloads/macos/) si `python3` n'est pas disponible, et GMP avec [Homebrew](https://brew.sh) : `brew install gmp`. Lancer ensuite :

```sh
python3 tester.py
```

### Autres commandes

| Action | Windows | Linux / macOS |
|---|---|---|
| Compiler et tout tester | `.\tester.cmd` | `python3 tester.py` |
| Compiler seulement | `.\tester.cmd all` | `python3 tester.py all` |
| Étude MAX_PRIME | `.\tester.cmd test-maxprime` | `python3 tester.py test-maxprime` |
| Vérifier le lancement après copie | `py -3 tests/test_lancement.py` | `python3 tests/test_lancement.py` |
| Supprimer les fichiers de compilation | `.\tester.cmd clean` | `python3 tester.py clean` |

Le lanceur recompile les programmes pour l'ordinateur utilisé. Il trouve les sources à partir de son propre emplacement, même si le chemin contient des espaces. Le compilateur peut être choisi avec la variable `CC`, par exemple `CC=clang python3 tester.py` sous Unix.

Il lance les tests de clefs de Nada, le main commun de Malek avec les tests d'Adam et de Loann, les cas de régression et les erreurs de fichiers. Le Base64 est comparé à celui de Python, puis à la commande `base64` si elle est installée. Les fichiers d'essai restent dans `build/tests` ; un échec de compilation ou de test renvoie un code non nul.

Le Makefile commun reste utilisable avec GCC ou Clang, GNU Make et les commandes Unix usuelles :

```sh
make
make test
make test-maxprime
```

`outils-locaux.txt` est une ancienne configuration facultative : si ce fichier existe, le lanceur lit le dossier des outils indiqué dedans. Il est ignoré par Git et n'est pas nécessaire aux autres membres.

Sous Windows, `tester.cmd` peut aussi utiliser l'ancien Makefile si Python est absent et que GCC, Make et les commandes Unix sont disponibles, comme dans w64devkit.

Les sources et les messages utilisent UTF-8. La console Windows est configurée au démarrage de chaque programme, même avec un lancement direct du `.exe`. Le lanceur Python et les compilateurs utilisent le même encodage. Les accents, majuscules accentuées et ligatures restent ainsi conservés.

Pour lire un log UTF-8 avec Windows PowerShell 5.1, préciser son encodage :

```powershell
Get-Content ".\build\tests\cles\diypg.log" -Encoding UTF8
```

Les exécutables sont dans `build/bin` :

- `test_phase1` : démonstration commune, puis tests des modules de chiffrement et Base64 ;
- `test_cles` : génération, affichage, sauvegarde, chargement et 1000 paires testées sur les 256 octets ;
- `test_base64` : tests de Loann seuls ;
- `test_regressions` : cas limites et aller-retour complet RSA/Base64 ;
- `test_io_errors` : erreurs de lecture, d'écriture et de fermeture simulées ;
- `test_maxprime` : étude du dépassement de capacité ;
- `test_gmp_rsa` : phase 2, puissance modulaire GMP, clefs avec N > 2^32 et chiffrement de blocs de 4 octets.

Sous Windows, les noms finissent par `.exe`. Le main crée ses fichiers dans le dossier depuis lequel il est lancé : utiliser le lanceur ou `make test` permet de garder tous ces fichiers dans `build/tests`. La commande `clean` enlève uniquement le dossier `build`.

## Fichiers à envoyer sur GitHub

Les sources, les tests, les Makefiles, les lanceurs, les fichiers Markdown, `.gitignore`, `.gitattributes` et `.github/workflows/tests.yml` font partie du projet. Les compilateurs, exécutables, objets, logs et clefs de test sont ignorés. Chaque membre recompile sur son ordinateur.

Le workflow GitHub Actions compile et teste sur Windows avec GCC, Linux avec GCC et macOS avec Clang. Il vérifie aussi une copie sans configuration locale dans un chemin contenant des espaces, puis relance l'étude MAX_PRIME. Ces contrôles démarreront au prochain push ou à la création d'une pull request. Ils ne publient aucun fichier et n'ont pas de droit d'écriture sur le dépôt.

## Fichiers conservés

- `phase1_nada/` : types et outils RSA, clefs, log et étude MAX_PRIME.
- `phase1_adam/` : chiffrement des octets, tableaux et fichiers, avec ses tests.
- `phase1_loann/` : Base64 des tableaux et fichiers, avec ses tests.
- `phase1_malek/` : main commun et fonctions d'appel aux autres modules.
- `tests/` : cas ajoutés après l'audit et relevé des nouvelles mesures.
- `phase2_nada/` : phase 2, RSA avec GMP et blocs de 4 octets (voir plus bas).

Les changements par rapport au dépôt de départ sont notés dans [MODIFICATIONS.md](MODIFICATIONS.md). Les commentaires des parties conservées ont été gardés.

## Conventions utilisées

Les types `rsaKey_t` et `keyPair_t` restent dans `phase1_nada/rsa_common_header.h`.

Pour chiffrer un tableau, on utilise `chiffrer_tableau` d'Adam. Un octet clair donne un entier RSA sur 8 octets little endian. Un tableau de n octets demande donc une sortie de 8*n octets. Le déchiffrement restitue n octets. Les longueurs sont explicites : les octets nuls et le texte UTF-8 ne sont pas coupés. Les tableaux d'entrée et de sortie ne doivent pas se recouvrir.

Les fichiers utilisent le même format binaire, sans en-tête. Les fonctions `chiffrer_fichier` et `dechiffrer_fichier` reçoivent la clef en paramètre. Elles préparent le résultat dans un fichier temporaire avant d'ouvrir la destination. Une entrée absente ou un chiffre invalide laisse ainsi la destination intacte. Une erreur pendant la copie finale peut laisser une sortie partielle ; le retour doit être vérifié.

`base64_encode` renvoie une chaîne terminée par `\0` et sa longueur sans ce terminateur. `base64_decode` reçoit une longueur et refuse les caractères invalides, le mauvais remplissage et les bits de remplissage non nuls. Une entrée vide est acceptée. Les buffers renvoyés sont à libérer avec `free`.

Les tableaux Base64 sont stricts, sans espaces. Pour les fichiers Base64, les espaces, tabulations et retours à la ligne sont retirés avant le décodage, afin de lire les sorties de la commande Unix. Une entrée Base64 invalide n'écrase pas la destination. Les conversions de fichiers Base64 chargent le fichier en mémoire ; une taille non représentable ou une allocation impossible produit une erreur.

Les conventions de retour gardent les interfaces existantes :

| Fonctions | Succès | Échec |
|---|---|---|
| `gen_keypair`, `save_keypair`, `load_keypair` de Nada | 0 | -1 |
| Fonctions de chiffrement d'Adam | `RSA_SUCCES`, soit 0 | Code négatif décrit dans `chiffrement.h` |
| Conversions de fichiers Base64 de Loann | 0 | -1 |
| `saveKeyPair`, `loadKeyPair`, `encryptFile`, `decryptFile` de Malek | 1 | 0 |
| `test_base64` de Loann | 1 | 0 |
| `tester_chiffrement_adam` | 0 | 1 |

Les fonctions conservées dans `phase1_malek/phase1.c` appellent les modules de Nada et d'Adam. Elles n'ont plus leur propre format de clefs ou de chiffre.

## Clefs et MAX_PRIME

La valeur par défaut reste **10000**. La génération courante accepte des bornes de **20 à 65536**. Les clefs doivent respecter `0 < E < N` et `255 < N <= 2^32`. Le chargement vérifie aussi l'aller-retour des 256 octets, sans modifier la paire de sortie en cas d'erreur.

Le format texte des clefs de Nada est conservé :

```text
DIYPG-KEYPAIR 1
PUB <exposant hexadécimal> <module hexadécimal>
PRIV <exposant hexadécimal> <module hexadécimal>
```

La sauvegarde contient les deux clefs. Sous Unix, elle demande les droits 0600 ; sous Windows, `chmod` agit sur les attributs du fichier et ne règle pas les ACL.

L'étude MAX_PRIME conserve la puissance modulaire sur 64 bits du sujet. Elle peut volontairement dépasser la borne courante et constater des déchiffrements faux. Cela explique le passage à GMP dans la phase suivante. Elle accepte une borne jusqu'à 5000000 et une graine facultative :

```sh
build/bin/test_maxprime 70000 2000 20261015
build/bin/test_cles 10000 20261015
```

`make test-maxprime` teste onze bornes avec 2000 paires chacune et la graine 20261015. Les résultats sont écrits dans `build/resultats_maxprime.txt`. Un relevé vérifié est gardé dans [tests/resultats_maxprime.txt](tests/resultats_maxprime.txt). Les anciens relevés de Nada restent dans son dossier et sont signalés comme mesures initiales.

Au-dessus de `2^32`, il y a un risque de dépassement, pas un échec systématique de toute paire. Le contre-exemple du rapport d'audit est vérifié dans les tests de régression. Les compteurs d'échec de l'étude ne sont donc pas ceux de `make test` : des échecs y sont attendus quand la borne devient trop grande.

## Vérifications

Les essais couvrent les clefs générées, les valeurs connues RSA/Base64, les 256 octets, l'UTF-8, les tableaux et fichiers vides, les fichiers binaires, les tailles autour de 4096 octets, les entrées tronquées, les clefs invalides, les Base64 incorrects et les erreurs d'E/S. L'aller-retour complet va jusqu'à 100003 octets. Sous Windows, les tests d'Adam comptent 123121 vérifications ; les tests ajoutés comptent 1570 vérifications et 13 contrôles d'E/S, dont 8 erreurs simulées. Le nombre de contrôles dépend du système car certains tests utilisent les protections mémoire de Windows.

Les tests de fichiers écrivent uniquement leurs données d'essai dans `build/tests` quand ils sont lancés par le Makefile. Les programmes de test renvoient un code non nul si un contrôle échoue.

La suite a été exécutée localement sous Windows x64 avec GCC 16.2.0. Les résultats des vérifications Windows, Linux et macOS sont à consulter dans l'onglet Actions du dépôt pour le commit testé.

## Phase 2 — RSA avec GMP

### Partie de Nada : `phase2_nada/`

Un bloc de 4 octets vaut jusqu'à 2^32 - 1, il faut donc N > 2^32. Dans cette zone, `a * a` déborde sur 64 bits et `puissance_mod_n` de la phase 1 se trompe (l'étude MAX_PRIME le montre). La phase 2 fait les calculs avec GMP ; les clefs restent des `rsaKey_t` (E et N sur 64 bits).

| Fichier | Contenu |
|---|---|
| `gmp_rsa.h`, `gmp_rsa.c` | `puissance_mod_n_gmp` (signature du sujet, `mpz_powm`), `gen_keys_gmp` (p et q de 17 à 31 bits, donc 2^32 < N < 2^64), `chiffrer_bloc_gmp`, `dechiffrer_bloc_gmp` |
| `convert_bytes.h`, `convert_bytes.c` | `convert_4byte2int` et `convertInt2uchar`, en **little endian** : `{0x78, 0x56, 0x34, 0x12}` donne `0x12345678` |
| `tests_gmp_rsa.c` | comparaison avec la phase 1 pour N ≤ 2^32, débordement de la phase 1 au-delà, ordre des octets, clefs, aller-retour des blocs, erreurs |

Les fonctions renvoient les codes d'Adam définis dans `phase1_adam/chiffrement.h` : `RSA_SUCCES`, `RSA_ERREUR_PARAMETRE`, `RSA_ERREUR_CLE` et `RSA_ERREUR_CHIFFRE`. Un bloc chiffré est un entier inférieur à N, stocké sur 8 octets (`RSA_BLOC_CHIFFRE_TAILLE`). Le générateur aléatoire de GMP utilisé pour les clefs n'est pas cryptographique.

Le sujet indique que les deux fonctions de conversion sont fournies sur Moodle. Ce fichier n'étant pas dans le dépôt, `convert_bytes.c` les remplace ; s'il est ajouté, supprimer `convert_bytes.c` et garder ses prototypes.

### Installer GMP

| Système | Commande |
|---|---|
| Ubuntu / Debian | `sudo apt install libgmp-dev` |
| macOS (Homebrew) | `brew install gmp` |
| Windows (MSYS2) | installer [MSYS2](https://www.msys2.org), puis dans son terminal : `pacman -S mingw-w64-x86_64-gmp` |

Les en-têtes et la bibliothèque sont cherchés dans les chemins par défaut du compilateur. Sinon, la variable facultative `GMP_PREFIX` indique le dossier qui contient `include/gmp.h` et `lib/` :

- sous macOS, si `GMP_PREFIX` n'est pas définie, le lanceur et le Makefile utilisent `brew --prefix gmp` ;
- sous Windows, `tester.cmd` utilise `C:\msys64\mingw64` si GMP y est installée, et ajoute son dossier `bin` au PATH pour trouver la DLL au lancement des tests ;
- ailleurs, par exemple : `GMP_PREFIX=/opt/gmp python3 tester.py` ou `make test GMP_PREFIX=/opt/gmp`.

### Lancer les tests

Les tests de la phase 2 sont compris dans les commandes habituelles, après ceux de la phase 1 :

```sh
python3 tester.py        # ou .\tester.cmd sous Windows
make test
```

Pour ne compiler et lancer que la phase 2 avec le Makefile :

```sh
make build/bin/test_gmp_rsa
mkdir -p build/tests/gmp && cd build/tests/gmp && ../../bin/test_gmp_rsa
```

Si GMP est introuvable, la compilation de `test_gmp_rsa` échoue et le lanceur renvoie vers cette section. Le workflow GitHub installe GMP sur les trois systèmes avant les tests.
