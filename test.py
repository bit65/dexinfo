#!/usr/bin/python
import pydexinfo

class t:
	def read(self, size):
		print "Reading %d" % size
		return " " * size

	def seek(self, pos, whence = 0):
		print "Seeking %d in %d" % (pos, whence)


#pydexinfo.dexinfo(t())
f = open("classes.dex", "rb")
pydexinfo.dexinfo(f)
f.close()
