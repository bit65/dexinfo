#!/usr/bin/python
import pydexinfo
import re

class t:
	def read(self, size):
		print "Reading %d" % size
		return " " * size

	def seek(self, pos, whence = 0):
		print "Seeking %d in %d" % (pos, whence)


#pydexinfo.dexinfo(t())
f = open("classes.dex", "rb")
s = pydexinfo.parse(f)
f.close()

print(set(re.findall("MethodVal (.+)", s)))
