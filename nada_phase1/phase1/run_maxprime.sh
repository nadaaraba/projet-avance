#!/bin/sh
# Lance l'étude de MAX_PRIME pour plusieurs valeurs et garde les résultats dans maxprime_results.txt
# Utilisation (dans le dossier phase1/, après "make") :   sh run_maxprime.sh
OUT=maxprime_results.txt
: > "$OUT"
for MAX in 1000 10000 30000 60000 65536 70000 80000 100000 200000 1000000 5000000; do
  ./test_maxprime "$MAX" 2000 | tee -a "$OUT"
done
echo "Résultats écrits dans $OUT"
