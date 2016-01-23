# sabir

A language detector.

## Purpose

This is a small library for detecting the natural language of written texts. I
wrote it because I needed to recognize transliterated Sanskrit texts, which
isn't possible with any language detection library out there.

The approach used is similar to that of
[`libcld2`](https://github.com/CLD2Owners/cld2), though simpler. Conceptually,
we first preprocess the text to remove non-alphabetic code points. Each
remaining character sequence is then padded on the left and the right with
`0xff` bytes (which cannot appear in valid UTF-8 strings), and, if the resulting
sequence is long enough, byte quadgrams are extracted from it and fed to a
multinomial Naive Bayes classifier. The string `Ô, café!`, for instance, is
turned into the following quadgrams (in Python3 notation):

    b'\xff\xc3\x94\xff'
    b'\xffcaf'
    b'caf\xc3'
    b'af\xc3\xa9'
    b'f\xc3\xa9\xff'

I've made two simplifying assumptions as concerns the Naive Bayes classifier:
priors are treated as if they were uniform (which is of course likely not to be
the case in practice), and the length of a document is a constant. Refinements
are certainly possible, but the resulting classifier is already good enough for
my purpose.

## Building

The library is available in source form, as an amalgamation. Compile `sabir.c`
together with your source code, and use the interface described in `sabir.h`. A
C11 compiler is required for compilation, which means either GCC or CLang on
Unix. You'll also need to link the compiled code to
[`utf8proc`](https://github.com/JuliaLang/utf8proc)

Two command-line tools are also included: a classification program, `sabir`, and
a script for creating and evaluating classification models, `sabir-train`.
Finally, a model file `model.sb` is included which recognizes English, French,
German, Italian, and transliterated Sanskrit. It was trained on a few megabytes
of Wikipedia dumps and on electronic texts from the [`Gretil
project`](http://gretil.sub.uni-goettingen.de/).

To compile and install all the above:

    $ make && sudo make install


## Usage

There is a concrete usage example in `example.c`. Compile this file with `make`,
and use the output binary like so:

    $ ./example < README.md
    en

Full details are given in `sabir.h`.

## Training

Provided you have enough data, it is possible (and desirable) to train a
specialized model. The script `sabir-train` can be used for that purpose. You
should first create one file per language, named after the language, and put all
the created files in some directory. Then, to get an idea of the accuracy of
your future classifier, call the script like so:

    $ sabir-train eval test/data/*
    # ...
    macro-precision: 99.078
    macro-recall: 99.076
    macro-F1: 99.077

If you're fine with the results, you can then create a model file with the
following:

    $ sabir-train dump test/data/* > my_model

The resulting model can be used with the C API, or with the command-line
classifier, e.g.:

    $ sabir --model=my_model README.md
    en

## References

I implemented the approach described in [`Cavnar and Trenkle,
1994`](http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.53.9367&rep=rep1&type=pdf)
some time ago before writing this library. This method seems to be the most
popular out there, perhaps because it is very simple. However, it requires to
create a ngram dictionary during classification, which means that a lot of
memory allocations are necessary to classify a single document. In contrast, my
current implementation only allocates a single chunk of memory, at the time a
model is loaded.

For a comparison of the existing approaches, see [`Baldwin and Lui,
2010`](http://www.aclweb.org/anthology/N10-1027).
