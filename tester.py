#!/usr/bin/env python3
"""Compile et teste les phases 1 a 3 avec Python et GCC ou Clang."""

import argparse
import base64
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys

from phase2_adam.tester_phase2 import verifier_reference
from phase3_adam.tester_contacts import reference_python as verifier_contacts

ROOT = Path(__file__).resolve().parent
BUILD = ROOT / "build"
BIN = BUILD / "bin"
EXT = ".exe" if os.name == "nt" else ""

RSA = ["phase1_nada/phase1/" + name + ".c"
       for name in ("rsa_tools", "bezout", "keys_io", "log")]
BASE64 = ["phase1_loann/base64.c", "phase1_loann/other_base64.c"]
COMMUN = RSA + ["phase1_adam/chiffrement.c"] + BASE64
GMP_RSA = ["phase2_nada/gmp_rsa.c", "phase2_nada/convert_bytes.c"]
FICHIERS_BLOCS = ["phase2_adam/chiffrement_blocs.c", "phase2_adam/fichiers_gmp.c"]
BASE64_V2 = ["phase2_loann/base64_v2.c"]
COMMANDES_NADA = ["phase3_interprete/table_clefs.c", "phase3_nada/commandes_nada.c"]
INTERPRETE = ["phase3_interprete/interprete.c"]
CONTACTS_ADAM = ["phase3_adam/contacts.c", "phase3_adam/contacts_io.c",
                 "phase3_adam/contacts_commandes.c"]
PROGRAMMES = {
    "test_phase1": COMMUN + ["phase1_adam/tests_chiffrement.c",
                           "phase1_loann/tests_base64.c",
                           "phase1_malek/phase1.c", "phase1_malek/main_phase1.c"],
    "test_cles": RSA + ["phase1_nada/phase1/test_cles.c"],
    "test_maxprime": RSA + ["phase1_nada/phase1/test_maxprime.c"],
    "test_base64": BASE64 + ["phase1_loann/tests_base64.c",
                           "phase1_loann/main_test_loann.c"],
    "test_regressions": COMMUN + ["phase1_malek/phase1.c", "tests/test_regressions.c"],
    # Ce test inclut lui-meme keys_io.c et base64.c pour simuler les erreurs.
    "test_io_errors": [RSA[0], RSA[1], RSA[3], BASE64[1], "tests/test_io_errors.c"],
    "test_gmp_rsa": RSA + GMP_RSA + ["phase2_nada/tests_gmp_rsa.c"],
    "test_fichiers_blocs": GMP_RSA + FICHIERS_BLOCS + ["phase2_adam/tests/test_fichiers_blocs.c"],
    "test_blocs_io": ["phase2_adam/tests/test_blocs_io.c"],
    "test_phase2_loann": BASE64 + GMP_RSA + FICHIERS_BLOCS +
                        ["phase2_loann/base64_v2.c", "tests/test_phase2_loann.c"],
    "test_interprete": RSA + GMP_RSA + FICHIERS_BLOCS + BASE64 + BASE64_V2 + COMMANDES_NADA + INTERPRETE
                       + CONTACTS_ADAM + ["phase3_adam/contacts_interpreteur.c",
                                          "phase3_interprete/tests/test_interprete.c"],
    "diypg": RSA + GMP_RSA + FICHIERS_BLOCS + BASE64 + BASE64_V2 + COMMANDES_NADA + INTERPRETE
             + CONTACTS_ADAM + ["phase3_adam/contacts_interpreteur.c", "phase3_interprete/main_diypg.c"],
    "test_integration": RSA + GMP_RSA + FICHIERS_BLOCS + BASE64 + BASE64_V2 + COMMANDES_NADA
                        + CONTACTS_ADAM + ["phase3_adam/contacts_interpreteur.c",
                                           "tests/test_integration.c"],
    "test_commandes_nada": RSA + GMP_RSA + FICHIERS_BLOCS + BASE64 + BASE64_V2 + COMMANDES_NADA
                           + CONTACTS_ADAM + ["phase3_adam/contacts_interpreteur.c",
                                              "phase3_nada/tests_commandes_nada.c"],
    "test_contacts": CONTACTS_ADAM + ["phase3_adam/contacts_interpreteur.c",
                                     "phase3_adam/tests/test_contacts.c"],
    "test_contacts_memoire": ["phase3_adam/tests/test_contacts_memoire.c"],
    "rejouer_contacts": CONTACTS_ADAM + ["phase3_adam/tests/rejouer_contacts.c"],
}
# Programmes des phases 2 et 3 qui ont besoin de la bibliotheque GMP.
AVEC_GMP = {"test_gmp_rsa", "test_fichiers_blocs", "test_phase2_loann", "test_commandes_nada",
            "test_integration", "test_interprete", "diypg"}


def configurer_affichage_utf8():
    if os.name == "nt":
        import ctypes
        ctypes.windll.kernel32.SetConsoleOutputCP(65001)
        ctypes.windll.kernel32.SetConsoleCP(65001)
    # Les sorties redirigees doivent aussi rester en UTF-8 sous Windows.
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")


def outils_locaux():
    dossiers = [ROOT / "w64devkit" / "bin"]
    config = ROOT / "outils-locaux.txt"
    if config.is_file():
        chemin = config.read_text(encoding="utf-8-sig").strip()
        if chemin:
            dossiers.append(ROOT / chemin)
    dossiers = [str(p) for p in dossiers if p.is_dir()]
    if dossiers:
        os.environ["PATH"] = os.pathsep.join(dossiers + [os.environ.get("PATH", "")])


def compilateur():
    choix = os.environ.get("CC", "").strip()
    if choix:
        # Un chemin contenant des espaces reste un seul argument.
        chemin = choix.strip('"')
        commande = [chemin] if Path(chemin).is_file() else shlex.split(choix)
        if commande and shutil.which(commande[0]):
            return commande
        raise RuntimeError("Compilateur CC introuvable : " + choix)
    for nom in ("gcc", "clang", "cc"):
        chemin = shutil.which(nom)
        if chemin:
            return [chemin]
    raise RuntimeError("GCC ou Clang est introuvable. Voir les installations dans README.md.")


def options_gmp():
    """Renvoie les options -I et -L de GMP, prises dans GMP_PREFIX ou chez Homebrew."""
    prefixe = os.environ.get("GMP_PREFIX")
    if prefixe is None:
        brew = shutil.which("brew")
        if brew:
            resultat = subprocess.run([brew, "--prefix", "gmp"], capture_output=True,
                                      text=True, timeout=30)
            if resultat.returncode == 0:
                prefixe = resultat.stdout.strip()
    if not prefixe:
        return [], []
    prefixe = Path(prefixe.strip('"'))
    if os.name == "nt":
        # La DLL de GMP doit etre trouvee au lancement des tests.
        os.environ["PATH"] = os.pathsep.join([os.environ.get("PATH", ""), str(prefixe / "bin")])
    return ["-I", str(prefixe / "include")], ["-L", str(prefixe / "lib")]


def compiler(noms):
    cc = compilateur()
    gmp_include, gmp_lib = options_gmp() if AVEC_GMP & set(noms) else ([], [])
    subprocess.run(cc + ["--version"], check=True, timeout=30)
    BIN.mkdir(parents=True, exist_ok=True)
    for nom in noms:
        print("\nCompilation : " + nom, flush=True)
        commande = cc + ["-std=c11", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                         "-finput-charset=UTF-8", "-fexec-charset=UTF-8",
                         "-I", str(ROOT)]
        if nom in AVEC_GMP:
            commande += gmp_include
        commande += [str(ROOT / source) for source in PROGRAMMES[nom]]
        if nom in AVEC_GMP:
            # Les bibliotheques viennent apres les sources pour l'editeur de liens GNU.
            commande += gmp_lib + ["-lgmp"]
        commande += ["-o", str(BIN / (nom + EXT))]
        # On recompile pour ne pas reutiliser un executable d'un autre ordinateur.
        try:
            subprocess.run(commande, cwd=ROOT, check=True, timeout=180)
        except subprocess.CalledProcessError:
            if nom in AVEC_GMP:
                print("\nGMP est-elle installee ? Voir la section phase 2 du README "
                      "(variable GMP_PREFIX).", file=sys.stderr, flush=True)
            raise


def lancer(nom, dossier, *arguments, **options):
    travail = BUILD / "tests" / dossier
    travail.mkdir(parents=True, exist_ok=True)
    print("\nTest : " + nom, flush=True)
    return subprocess.run([str(BIN / (nom + EXT)), *map(str, arguments)],
                          cwd=travail, check=True, timeout=180, **options)


def reference_base64():
    lancer("test_base64", "reference")
    dossier = BUILD / "tests" / "reference"
    source = (dossier / "test_source.bin").read_bytes()
    attendu = base64.b64encode(source)
    if (dossier / "test_out.b64").read_bytes() != attendu:
        raise RuntimeError("Le Base64 ne correspond pas a la reference Python.")
    print("Base64 : comparaison avec Python reussie.", flush=True)
    outil = shutil.which("base64")
    if outil:
        # L'entree standard fonctionne avec les versions GNU et macOS de base64.
        resultat = subprocess.run([outil], input=source, capture_output=True,
                                  check=True, timeout=30)
        if b"".join(resultat.stdout.split()) != attendu:
            raise RuntimeError("Le Base64 ne correspond pas a la commande du systeme.")
        print("Base64 : comparaison avec la commande du systeme reussie.", flush=True)
    else:
        print("Commande Unix base64 absente ; reference Python verifiee.", flush=True)


def rejouer_scenario():
    """Rejoue le script de commandes de la phase 3 et compare a la sortie attendue."""
    dossier = ROOT / "phase3_interprete" / "tests"
    resultat = lancer("diypg", "interprete", "../../../phase3_interprete/tests/scenario.txt",
                      stdout=subprocess.PIPE)
    obtenu = resultat.stdout.decode("utf-8").replace("\r\n", "\n")
    attendu = (dossier / "scenario_attendu.txt").read_text(encoding="utf-8").replace("\r\n", "\n")
    if obtenu != attendu:
        import difflib
        ecarts = difflib.unified_diff(attendu.splitlines(), obtenu.splitlines(),
                                      "attendu", "obtenu", lineterm="")
        raise RuntimeError("Le script de la phase 3 ne donne pas la sortie attendue :\n"
                           + "\n".join(ecarts))
    print("Phase 3 (interprete) : script rejoue conforme a la sortie attendue.", flush=True)


def tester():
    reference_base64()
    lancer("test_cles", "cles", 10000, 20261015)
    lancer("test_phase1", "commun")
    lancer("test_regressions", "regressions")
    lancer("test_io_errors", "io")
    print("\nTous les tests de la phase 1 sont passes.", flush=True)
    lancer("test_gmp_rsa", "gmp")
    lancer("test_fichiers_blocs", "fichiers_blocs")
    lancer("test_blocs_io", "blocs_io")
    verifier_reference(BUILD / "tests" / "fichiers_blocs")
    print("Phase 2 (Adam) : tous les tests sont OK.", flush=True)
    lancer("test_phase2_loann", "phase2_loann")
    print("\nTous les tests de la phase 2 sont passes.", flush=True)
    lancer("test_commandes_nada", "phase3")
    lancer("test_contacts", "phase3_adam")
    lancer("test_contacts_memoire", "phase3_adam")
    lancer("rejouer_contacts", "phase3_adam", ROOT / "phase3_adam/tests/scenario_contacts.txt")
    verifier_contacts(BUILD / "tests/phase3_adam/contacts_reference.bin")
    print("Phase 3 (Adam) : tous les tests sont OK.", flush=True)
    print("\nTous les tests de la phase 3 sont passes.", flush=True)
    lancer("test_interprete", "interprete")
    rejouer_scenario()
    lancer("test_integration", "integration")
    print("\nTous les tests d'integration sont passes.", flush=True)


def etude_maxprime():
    sortie = BUILD / "resultats_maxprime.txt"
    with sortie.open("w", encoding="utf-8", newline="\n") as fichier:
        fichier.write("Graine 20261015, 2000 paires par borne\n")
        for borne in (1000, 10000, 30000, 60000, 65536, 70000, 80000,
                      100000, 200000, 1000000, 5000000):
            resultat = lancer("test_maxprime", "maxprime", borne, 2000, 20261015,
                              stdout=subprocess.PIPE)
            texte = resultat.stdout.decode("utf-8", errors="replace").replace("\r\n", "\n")
            fichier.write(texte)
            print(texte, end="", flush=True)
    print("\nResultats : " + str(sortie), flush=True)


def nettoyer():
    # On refuse de suivre un lien qui ferait sortir du dossier du projet.
    if BUILD.is_symlink() or BUILD.resolve() != ROOT / "build":
        raise RuntimeError("Le dossier build pointe ailleurs ; nettoyage refuse.")
    if BUILD.exists():
        shutil.rmtree(BUILD)
    print("Dossier build nettoye.")


def main():
    configurer_affichage_utf8()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", nargs="?", default="test",
                        choices=("all", "test", "test-reference", "test-maxprime", "clean"))
    action = parser.parse_args().action
    if action == "clean":
        nettoyer()
        return
    outils_locaux()
    noms = {"test-reference": ["test_base64"], "test-maxprime": ["test_maxprime"]}
    compiler(noms.get(action, PROGRAMMES))
    if action == "test":
        tester()
    elif action == "test-reference":
        reference_base64()
    elif action == "test-maxprime":
        etude_maxprime()


if __name__ == "__main__":
    if sys.version_info < (3, 9):
        sys.exit("Python 3.9 minimum est necessaire.")
    try:
        main()
    except subprocess.CalledProcessError as erreur:
        print("\nEchec de la commande : " + str(erreur.cmd), file=sys.stderr)
        sys.exit(erreur.returncode if erreur.returncode > 0 else 1)
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as erreur:
        print("\nErreur : " + str(erreur), file=sys.stderr)
        sys.exit(1)
