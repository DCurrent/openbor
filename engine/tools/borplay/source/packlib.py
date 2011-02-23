# Copyright (c) 2009 Bryan Cain ("Plombo")
# Class and functions to read .PAK files.

import struct
from cStringIO import StringIO

class PackFileReader(object):
	''' Represents a BOR packfile. '''
	
	files = dict() # the index holding the location of each file
	packfile = None # the file object
	
	def __init__(self, fp):
		'''fp is a file path (string) or file-like object (file, StringIO, 
		etc.) in binary read mode'''
		if isinstance(fp, str):
			self.packfile = open(fp, 'rb')
		else:
			self.packfile = fp
		
		self.read_index()
	
	# reads the packfile's index into self.files
	def read_index(self):
		f = self.packfile
		
		# read through file
		tmp = True # placeholder that doesn't evaluate to false
		while tmp: tmp = f.read(8192)
		
		# read index start postition and seek there
		f.seek(-4, 1)
		endpos = f.tell()
		f.seek(struct.unpack('<I', f.read(4))[0])
		while f.tell() < endpos:
			ssize, fpos, fsize = struct.unpack('<III', f.read(12))
			name = f.read(ssize-12).strip('\x00').replace('\\', '/').lower()
			self.files[name] = fpos, fsize
	
	# reads a file with its full path.
	def read_file(self, filename):
		'''Returns a file-like object for the file or None if the file isn't 
		contained in this packfile.
		
		This method takes the full path starting with "data/" as a parameter.'''
		
		key = filename.replace('\\', '/').lower().strip('\x00').strip()
		if key not in self.files.keys(): return None
		
		start, size = self.files[key]
		self.packfile.seek(start)
		f = StringIO()
		bytesrem = size
		while bytesrem >= 8192:
			f.write(self.packfile.read(8192))
			bytesrem -= 8192
		if bytesrem: f.write(self.packfile.read(bytesrem))
		f.seek(0)
		return f
	
	def find_file(self, filename):
		'''Returns a file-like object for the file or None if the file isn't 
		contained in this packfile.
		
		This method searches for the file by its filename.'''
		filename = filename.lower().strip()
		start, size = None, None
		for key in self.files.keys():
			if key.endswith(filename):
				return self.read_file(key)
		
		return None # file not found if it gets to this point
	
	def list_music_files(self):
		'''Lists the BOR files in the packfile.'''
		borfiles = []
		for key in self.files.keys():
			if key.endswith('.bor'): borfiles.append(key)
		borfiles.sort()
		for key in borfiles: print key
		

def get_file(pak, borfile):
	'''Prevents a need to directly use PackFileReader when you only want to get 
	one file, like in borplay and bor2wav. Returns a file-like object.'''
	
	rdr = PackFileReader(pak)
	if ('/' not in borfile) and ('\\' not in borfile): # only the filename is given; search for the file
		return rdr.find_file(borfile)
	else: # full path given
		return rdr.read_file(borfile)
	


# For testing
if __name__ == '__main__':
	rdr = PackFileReader('K:/BOR/OpenBOR/Paks/BOR.PAK')
	#keys = rdr.files.keys(); keys.sort()
	#print '\n'.join(keys)
	#print rdr.read_file('data/chars/yamazaki/yamazaki.txt').read()
	#print rdr.find_file('yamazaki.txt').read()
	rdr.list_music_files()

