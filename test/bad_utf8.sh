#!/usr/bin/env sh

VG="valgrind --leak-check=full --error-exitcode=1"
SABIR=./sabir

./sabir-train dump test/data/* > test/model.tmp
$VG ./sabir -mtest/model.tmp test/bad_utf8.txt

rm test/model.tmp

exit 0
