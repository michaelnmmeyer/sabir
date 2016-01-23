#!/usr/bin/env python3

import os, sys, imp, subprocess, random, codecs

NUM_TEST_DOCS = 100
MAX_DOC_LEN = 600
CHARS = "abcde\nfgh ijklmn\topqrst uvwxy\rzàâéèê îûô,?!“”‘’ "

this_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(this_dir)
 
sabir_py = imp.load_source("sabir-train", os.path.join(parent_dir, "sabir-train"))
sabir_c = os.path.join(parent_dir, "sabir")

train_corpus = sabir_py.load_corpora(sys.argv[1:])

def make_py_classifier():
   classifier, _, _ = sabir_py.train_vector(train_corpus)
   def classify(path):
      doc = list(sabir_py.iter_ngrams(path))
      guess, infos = classifier(doc)
      return infos, guess
   return classify

def make_c_classifier():
   model = os.path.join(this_dir, "model.tmp")
   with open(model, "w") as fp:
      sabir_py.mkmodel(train_corpus, fp)
      pass
   def classify(path):
      p = subprocess.Popen([sabir_c, "-vm", model, path], stdout = subprocess.PIPE)
      infos = []
      for line in p.stdout:
         fields = line.decode().strip().split()
         if len(fields) == 1:
            assert not p.stdout.readline()
            return infos, fields[0]
         else:
            fields[0] = codecs.decode(fields[0], "hex")
            fields[2] = int(fields[2])
            fields[3] = float.fromhex(fields[3])
            infos.append(tuple(fields))
      raise Exception
   return classify

def make_doc(path):
   doc_len = random.randint(0, MAX_DOC_LEN)
   with open(path, "w") as fp:
      for i in range(doc_len):
         fp.write(CHARS[random.randint(0, len(CHARS) - 1)])

def dump_ret(path, guessed_lang, infos):
   with open(os.path.join(this_dir, path), "w") as fp:
      for ngram, lang, hash, prob in infos:
         prob = prob.hex()
         print("%r %s %d %s" % (ngram, lang, hash, prob), file=fp)
      print(guessed_lang, file=fp)
      

py_classify = make_py_classifier()
c_classify = make_c_classifier()

path = os.path.join(this_dir, "doc.tmp")
for n in range(1, NUM_TEST_DOCS + 1):
   print("%d/%d" % (n, NUM_TEST_DOCS))
   make_doc(path)
   py_infos, py_lang = py_classify(path)
   c_infos, c_lang = c_classify(path)
   if py_lang != c_lang or py_infos != c_infos:
      dump_ret("py_ret.tmp", py_lang, py_infos)
      dump_ret("c_ret.tmp", c_lang, c_infos)
      raise Exception("fail!")

for file in os.listdir(this_dir):
   if file.endswith(".tmp"):
      os.remove(os.path.join(this_dir, file))
