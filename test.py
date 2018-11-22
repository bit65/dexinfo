#!/usr/bin/python
import pydexinfo

f = open("/home/giliy/libsearch/examples/cache/classes.dex", "rb")
pydexinfo.dexinfo(f)
f.close()
