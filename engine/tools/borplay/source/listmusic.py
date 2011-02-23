#!/usr/bin/env python
# Copyright (c) 2009 Bryan Cain ("Plombo")
# Lists all of the music files in a PAK file.

import packlib
import sys

if len(sys.argv) != 2:
	print 'listmusic v0.1'
	print 'written by Plombo'
	sys.exit("Usage: %s <file.pak>" % sys.argv[0])

try: pak = packlib.PackFileReader(sys.argv[1])
except IOError:
	sys.exit('File not found!')
pak.list_music_files()

