# Phase 3 — contacts V2 (Adam)

J'ai réalisé la gestion des contacts et la recherche de leurs clefs
publiques avec la notation contact/clef.

## Utilisation du module

Un annuaire s'initialise avec contacts_initialiser, ou avec {NULL}, puis se
libère avec contacts_liberer. Les structures des listes restent internes.

contacts.h propose les fonctions pour :
- ajouter, modifier, chercher et supprimer un contact ;
- ajouter une clef publique par identifiant de contact ou par nom de famille ;
- supprimer une seule clef avec contact/clef ;
- retrouver une clef publique et vérifier son type avec contacts_resoudre ;
- afficher les contacts et leurs identifiants de clefs.

Un identifiant fait de 1 à 32 octets, sans espace, caractère de contrôle ni /.
Nom et prénom acceptent 127 octets chacun, et le commentaire 511 octets.
Les textes UTF-8 et les espaces sont conservés ; ces limites sont en octets.
Les identifiants des contacts sont uniques. Un identifiant de clef est unique
dans son contact, mais peut être réutilisé dans un autre contact.

Pour addkeys, un identifiant exact est prioritaire sur un nom de famille.
Si plusieurs contacts portent le même nom, utiliser leur identifiant.
Les clefs sont publiques, avec un type crypt ou sign. L'ajout vérifie
0 < E < N et N > 1 ; ce n'est pas une preuve de validité mathématique de la clef.
Une clef crypt doit en plus avoir N > 2^32 - 1, sinon le chiffrement par blocs
de 4 octets la refuserait.

Une référence s'écrit contact/clef, ou contact seul si le contact n'a qu'une
clef (interprète.pdf : « celui de la clef s'il en a plusieurs »). Un contact seul
avec plusieurs clefs donne CONTACTS_ERREUR_AMBIGU. rmkeys exige contact/clef.

## Raccordement à l'interpréteur

Après avoir découpé une ligne, Loann peut appeler :

    contacts_executer(&contacts, argc, (const char *const *)argv, entree, sortie);

Même convention que commande_t de phase3_nada/interpreteur.h : argv[0] est
le nom de la commande, comme pour main.
Les flux sont généralement stdin et stdout. Ils peuvent aussi être des
fichiers pour rejouer un scénario.

Les commandes prises en charge sont :

| Commande | Effet |
| --- | --- |
| listcontacts [id ou nom] [nom] | Liste les contacts et leurs clefs |
| addcontact id | Demande le nom, le prénom et le commentaire |
| modifycontact id | Affiche la fiche, puis le menu de modification validé par 0 |
| addkeys id ou nom | Demande l'identifiant, le type, E et N de la clef |
| rmcontact id | Supprime le contact et toutes ses clefs |
| rmkeys contact/clef | Supprime seulement cette clef |

Dans le menu modifycontact, une valeur vide efface le champ choisi.
Les modifications ne sont enregistrées qu'au choix 0. Une saisie incomplète
ne modifie pas le contact. E et N sont saisis en hexadécimal, avec ou sans 0x.

Le retour 0 indique un succès. Le retour 1, CONTACTS_COMMANDE_INCONNUE, signifie
que la commande doit être traitée par l'interpréteur V1. Les autres retours
sont négatifs, de -20 à -30, pour ne pas se confondre avec les codes RSA_*
(-1 à -5) et CMD_* (-10 à -14) ; contacts_message_erreur donne leur texte.
La commande show est traitée par cmd_show dans phase3_nada.

Pour chiffrer vers un contact, contacts_resoudre avec CONTACT_CRYPT fournit
la clef publique de contact/clef. Une clef de signature est refusée dans ce cas.
Les commandes crypt, uncrypt, show et savepub de Nada (phase3_nada) utilisent
déjà contacts_resoudre_entree : table_clefs_t.contacts pointe vers l'annuaire.
Dans contacts_interpreteur.h, contacts_resoudre_entree fournit une copie au
format entree_clef_t du header proposé pour l'interpréteur. Elle traduit le
type, place l'identifiant de la clef dans id et marque a_priv à false. En cas
de référence mal formée, inconnue ou ambiguë, la fonction renvoie le code CMD
correspondant (CMD_ERREUR_SYNTAXE, CMD_ERREUR_ID_INCONNU, CMD_ERREUR_AMBIGU).
L'interpréteur peut ainsi utiliser la partie publique pour crypt, show et savepub.
Pour uncrypt, a_priv permet de signaler que la clef privée est absente.

Je propose ces points d'entrée pour raccorder mon module aux structures
et à l'analyseur de l'interpréteur de Loann.

## Sauvegarde et chargement

contacts_sauver écrit une section sur un FILE déjà ouvert en binaire.
contacts_charger lit cette section et laisse le flux positionné juste après.
Les fonctions n'ouvrent ni ne ferment les fichiers.

Format de la section :
- huit octets DYCONT01, puis le nombre de contacts sur quatre octets ;
- pour chaque contact : identifiant, nom, prénom, commentaire, nombre de clefs ;
- pour chaque clef : identifiant, type sur un octet (1 crypt, 2 sign), E puis N.

Chaque texte est précédé de sa longueur sur deux octets, sans terminateur NUL.
Le nombre de clefs est sur quatre octets ; E et N sont sur huit octets.
Tous les entiers sont little endian, sans dépendre de sizeof(unsigned long).

Le chargement refuse les champs trop longs, les NUL dans les textes, les types
invalides, les clefs mal formées, les doublons et les sections tronquées.
Il construit un annuaire temporaire : l'ancien n'est remplacé qu'en cas de
succès. Les paramètres de sortie de recherche restent aussi intacts en cas d'erreur.

Cette section n'est pas chiffrée par le module de contacts. Loann doit l'inclure
dans sa sauvegarde protégée, puis vérifier le vidage et la fermeture du fichier.
Pour que le chargement global soit lui aussi atomique, toutes ses sections
doivent être lues dans des structures temporaires avant le remplacement final.

## Compiler et tester

Depuis la racine, avec Python 3.9+ et GCC ou Clang :

    python3 phase3_adam/tester_contacts.py

Ou avec GNU Make :

    make -C phase3_adam test

Dans l'environnement Windows local :

    .\.adam-local\executer.cmd python phase3_adam\tester_contacts.py

Aucune bibliothèque supplémentaire n'est nécessaire pour les contacts.
Les programmes et fichiers de test vont dans build/adam_contacts.
À la racine, make test et python tester.py lancent aussi les tests des contacts.

## Vérifications

- Gestion des contacts et des clefs, noms ambigus, suppressions, types,
  recherches contact/clef, menus, saisies invalides et sauvegardes.
- Chaque coupure possible d'une section de test est refusée sans perdre
  l'annuaire chargé auparavant.
- 30 contrôles avec échecs d'allocation simulés, dont le nettoyage de
  l'annuaire temporaire et l'absence d'allocations restantes.
- Un scénario de commandes est rejoué depuis tests/scenario_contacts.txt.
- Python relit indépendamment le format binaire et vérifie les textes
  accentués, les types et les valeurs de clefs sur 64 bits.

tests/rejouer_contacts.c rejoue les commandes V2 directement sur l'annuaire.
