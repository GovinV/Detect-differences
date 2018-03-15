#!/bin/sh

#
# Quelques tests pour vérifier les options/paramètres
#

TEST=$(basename $0 .sh)

TMP=/tmp/$TEST
LOG=$TEST.log
V=${VALGRIND}		# appeler avec la var. VALGRIND à "" ou "valgrind -q"

exec 2> $LOG
set -x

fail ()
{
    echo "==> Échec du test '$TEST' sur '$1'."
    echo "==> Log : '$LOG'."
    echo "==> Exit"
    exit 1
}

DN=/dev/null

# tests élémentaires sur les options

./detecter					&& fail "pas d'arg"

./detecter -i1 -l 1 cat $DN			|| fail "syntaxe -i1 -l 1"

./detecter -l1 -i 1 cat $DN			|| fail "syntaxe -l 1 -i1"

./detecter -i 0 cat $DN				&& fail "intervalle nul"

./detecter -i -1 cat $DN			&& fail "intervalle négatif"

./detecter -l -1 cat $DN			&& fail "limite négative"

./detecter -i1 -l1				&& fail "pas de cmd"

./detecter -c -i1 -l1 cat $DN > $DN		|| fail "syntaxe -c"

./detecter -t %S -c -i1 -l1 cat $DN > $DN	|| fail "syntaxe -t %S"

exit 0
