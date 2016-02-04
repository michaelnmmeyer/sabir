#!/usr/bin/env sh

# Checks that the segmentation of the input text doesn't change with varying
# buffer sizes. Care must be taken to buffer incomplete UTF-8 sequences at the
# end of a chunk. In practice, if the buffer size is large enough, this
# probably doesn't improve classification performance by much. But we still do
# it for the sake of correctness.

SABIR="valgrind --leak-check=full --error-exitcode=1 ./sabir"

# Include sequences of all possible length.
S="saṃvéartāmaṇ🠺ḍalānte"

set -o errexit

# Reference.
echo $S | $SABIR -vm model.sb > test/trunc.tmp

for buf_size in 1 2 3 4 5; do
   echo $S | SB_BUF_SIZE=$buf_size $SABIR -vm model.sb | cmp test/trunc.tmp
done

rm test/trunc.tmp

exit 0
