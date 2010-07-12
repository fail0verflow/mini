#!/usr/bin/env python

import sys, os, struct

loaderfile = sys.argv[1]
elffile = sys.argv[2]
outfile = sys.argv[3]

data = open(loaderfile,"rb").read()

hdrlen, loaderlen, elflen, arg = struct.unpack(">IIII",data[:16])

if hdrlen < 0x10:
	print "ERROR: header length is 0x%x, expected at least 0x10"%hdrlen
	sys.exit(1)

loaderoff = hdrlen
elfoff = loaderoff + loaderlen
elfend = elfoff + elflen

hdr = data[:hdrlen]
loader = data[loaderoff:elfoff]

elf = open(elffile,"rb").read()

if elflen > 0:
	print "WARNING: loader already contains ELF, will replace"

elflen = len(elf)

if loaderlen < len(loader):
	print "ERROR: loader is larger than its reported length"
	sys.exit(1)

if loaderlen > len(loader):
	print "Padding loader with 0x%x zeroes"%(loaderlen-len(loader))
	loader += "\x00"*(loaderlen-len(loader))

newdata = struct.pack(">IIII", hdrlen, loaderlen, elflen, 0) + hdr[16:]
newdata += loader
newdata += elf

print "Header: 0x%x bytes"%(hdrlen)
print "Loader: 0x%x bytes"%(loaderlen)
print "ELF:    0x%x bytes"%(elflen)

f = open(outfile,"wb")
f.write(newdata)
f.close()
