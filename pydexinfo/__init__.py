from pydexinfo import *

class filewrapper:
	def __init__(self, f):
		self.data = f.read()
		self.pos = 0

	def read(self, size):
		d = self.data[self.pos:self.pos + size]
		self.pos += size

		return d

	def seek(self, offset, whence = 0):
		if (whence == 0):
			self.pos = offset
		elif (whence == 1):
			self.pos += offset
		else:
			self.pos = len(self.data) - pos

def parse(filename, verbose = False):
    return dexinfo(filewrapper(filename), verbose)
