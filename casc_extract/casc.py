# CASC file formats, based on the work of Caali et al. @ http://www.ownedcore.com/forums/world-of-warcraft/world-of-warcraft-model-editing/471104-analysis-of-casc-filesystem.html
import os, sys, mmap, md5, stat, struct, zlib, glob, re, urllib2

import jenkins

_BLTE_MAGIC = 'BLTE'
_CHUNK_DATA_OFFSET_LEN = 4
_CHUNK_HEADER_LEN = 4
_CHUNK_HEADER_2_LEN = 8
_CHUNK_SUM_LEN = 16
_MARKER_LEN = 1
_BLOCK_DATA_SIZE = 65535

_NULL_CHUNK = 0x00
_COMPRESSED_CHUNK = 0x5A
_UNCOMPRESSED_CHUNK = 0x4E

class BLTEChunk(object):
	def __init__(self, id, chunk_length, output_length, md5s):
		self.id = id
		self.chunk_length = chunk_length
		self.output_length = output_length
		self.sum = md5s
		
		self.output_data = ''

	def extract(self, data):
		if len(data) != self.chunk_length:
			self.options.parser.error('Invalid data length for chunk#%d, expected %u got %u' % (
				self.id, self.chunk_length, len(data)))
			return False
		
		type = ord(data[0])
		if type not in [ _NULL_CHUNK, _COMPRESSED_CHUNK, _UNCOMPRESSED_CHUNK ]:
			self.options.parser.error('Unknown chunk type %#x for chunk%d' % (type, self.id))
			return False
		
		if type != 0x00:
			self.__verify(data)
		
		return self.__decompress(data)
	
	def __decompress(self, data):
		type = ord(data[0])
		if type == _NULL_CHUNK:
			self.output_data = ''
		elif type == _UNCOMPRESSED_CHUNK:
			self.output_data = data[1:]
		else:
			dc = zlib.decompressobj()
			uncompressed_data = dc.decompress(data[1:])
			if len(dc.unused_data) > 0:
				self.options.parser.error('Unused %d bytes of compressed data in chunk%d' % (len(dc.unused_data), self.id))
				return False
			
			if len(uncompressed_data) != self.output_length:
				self.options.parser.error('Output chunk data length mismatch in chunk %d, expected %d got %d' % (
					self.id, self.output_length, len(uncompressed_data)))
				return False
			
			self.output_data = uncompressed_data

		return True

	def __verify(self, data):
		md5s = md5.new(data).digest()
		if md5s != self.sum:
			print >>sys.stderr, 'Chunk%d of type %#x fails verification, expects %s got %s' % (
				self.id, ord(data[0]), self.sum.encode('hex'), md5s.encode('hex'))
			return False
			
		return True

class BLTEFile(object):
	def __init__(self, extractor):
		self.data = extractor
		self.offset = 0
		self.chunks = []
		self.output_data = ''
		self.extract_status = True
	
	def add_chunk(self, length, c_length, md5s):
		self.chunks.append(BLTEChunk(len(self.chunks), length, c_length, md5s))
	
	def __read(self, bytes):
		if isinstance(self.data, BLTEExtract):
			return self.data.fd.read(bytes)
		else:
			nibble = self.data[self.offset:self.offset + bytes]
			self.offset += bytes
			return nibble
	
	def __seek(self, offset, pos):
		if isinstance(self.data, BLTEExtract):
			self.data.fd.seek(offset, pos)
		else:
			if pos == os.SEEK_CUR:
				self.offset += offset
			elif pos == os.SEEK_SET:
				self.offset = offset
			elif pos == os.SEEK_END:
				self.offset = len(self.data) + offset
	
	def __tell(self):
		if isinstance(self.data, BLTEExtract):
			return self.data.fd.tell()
		else:
			return self.offset

	def __extract_direct(self):
		type = ord(self.__read(_MARKER_LEN))
		if type != _COMPRESSED_CHUNK:
			self.options.parser.error('Direct extraction only supports compressed data, was given %#x' % type)
			return False
		
		# We don't know where the compressed data ends, so read until it's done
		compressed_data = self.__read(_BLOCK_DATA_SIZE)
		dc = zlib.decompressobj()
		while len(compressed_data) > 0:
			self.output_data += dc.decompress(compressed_data)
			if len(dc.unused_data) > 0:
				# Compressed segment ended at some point, rewind file position
				self.__seek(-len(dc.unused_data), os.SEEK_CUR)
				break
			
			# More data ..
			compressed_data = self.__read(_BLOCK_DATA_SIZE)
		
		return True

	def extract(self):
		pos = self.__tell()
		
		if self.__read(4) != _BLTE_MAGIC:
			self.options.parser.error('Invalid BLTE magic in file')
		
		chunk_data_offset = struct.unpack('>I', self.__read(_CHUNK_DATA_OFFSET_LEN))[0]
		if chunk_data_offset == 0:
			if not self.__extract_direct():
				return False
		else:
			unk_1, cc_b1, cc_b2, cc_b3 = struct.unpack('BBBB', self.__read(_CHUNK_HEADER_LEN))
			if unk_1 != 0x0F:
				self.options.parser.error('Unknown magic byte %d %#x in BLTE @%#.8x' % (unk_1, self.__tell() - _CHUNK_HEADER_LEN))
				return False
			
			n_chunks = (cc_b1 << 16) | (cc_b2 << 8) | cc_b3
			if n_chunks == 0:
				return False
			
			# Chunk information
			for chunk_id in xrange(0, n_chunks):
				c_len, out_len = struct.unpack('>II', self.__read(_CHUNK_HEADER_2_LEN))
				chunk_sum = self.__read(_CHUNK_SUM_LEN)

				#print 'Chunk#%d@%u: c_len=%u out_len=%u sum=%s' % (chunk_id, self.__tell() - 24, c_len, out_len, chunk_sum.encode('hex'))
				self.add_chunk(c_len, out_len, chunk_sum)
			
			# Read chunk data
			sum_in_file = 0
			for chunk_id in xrange(0, n_chunks):
				chunk = self.chunks[chunk_id]

				#print 'Chunk#%d@%u: Data extract len=%u, total=%u' % (chunk_id, self.__tell(), chunk.chunk_length, sum_in_file)
				data = self.__read(chunk.chunk_length)
				if not chunk.extract(data):
					self.extract_status = False

				self.output_data += chunk.output_data
				sum_in_file += len(chunk.output_data)

		return True
			
	def verify(self, md5s):
		pass

class BLTEExtract(object):
	def __init__(self, options):
		self.options = options

	def open(self, data_file):
		if not os.access(data_file, os.R_OK):
			self.options.parser.error('File %s not readable.' % data_file)
			return False
		
		self.fsize = os.stat(data_file).st_size
		if self.fsize == 0:
			self.options.parser.error('File %s is empty.' % data_file)
			return False
		
		self.fdesc = open(data_file, 'rb')
		self.fd = mmap.mmap(self.fdesc.fileno(), 0, access = mmap.ACCESS_READ)
	
		return True
	
	def close(self):
		if self.fd:
			self.fd.close()
		
		if self.fdesc:
			self.fdesc.close()
	
	def __extract_file(self):
		file = BLTEFile(self)
		if not file.extract():
			return None
		
		return file
	
	def extract_data(self, data):
		file = BLTEFile(data)
		if not file.extract():
			return None
		
		return file.output_data
	
	def extract_file(self, file_name):
		if not self.open(file_name):
			return False
		
		if not os.access(self.options.output, os.W_OK):
			self.options.parser.error('Output file %s is not writeable' % self.options.output)
			return False
		
		file = self.__extract_file()
		if not file:
			return False
		
		self.close()
		
		return True
	
	def extract_data(self, file_key, file_md5sum, data_file_number, data_file_offset, blte_file_size):
		path = os.path.join(self.options.data_dir, 'Data', 'data', 'data.%03u' % data_file_number)

		if not self.open(path):
			return None
		
		self.fd.seek(data_file_offset, os.SEEK_SET)
		
		key = self.fd.read(16)
		blte_len = struct.unpack('<I', self.fd.read(4))[0]
		if key[::-1] != file_key:
			self.options.parser.error('Invalid file key for %s, expected %s, got %s' % (file_output, file_key.encode('hex'), key.encode('hex')))
		
		if blte_len != blte_file_size:
			self.options.parser.error('Invalid file length, expected %u got %u' % (blte_file_size, blte_len))
		
		# Skip 10 bytes of unknown data
		self.fd.seek(10, os.SEEK_CUR)
		
		file = self.__extract_file()
		if not file:
			return None
		
		file_md5 = md5.new(file.output_data).digest()
		if file_md5sum and file_md5sum != file_md5:
			self.options.parser.error('Invalid md5sum for extracted file, expected %s got %s' % (file_md5sum.encode('hex'), file_md5.encode('hex')))
		
		self.close()
		
		return file.output_data
	
	def extract_file(self, file_key, file_md5sum, file_output, data_file_number, data_file_offset, blte_file_size):
		output_path = ''
		if file_output:
			output_path = os.path.join(self.options.output, file_output)
		else:
			if file_key:
				output_path = os.path.join(self.options.output, file_key.encode('hex'))
			elif file_md5sum:
				output_path = os.path.join(self.options.output, file_md5sum.encode('hex'))
		
		output_dir = os.path.dirname(os.path.abspath(output_path))
		try:
			if not os.path.exists(output_dir):
				os.makedirs(output_dir)
		except os.error, e:
			self.options.parser.error('Output "%s" is not writable: %s' % (output_path, e.strerror))
			
		data = self.extract_data(file_key, file_md5sum, data_file_number, data_file_offset, blte_file_size)
		if not data:
			return False
		
		try:
			with open(output_path, 'wb') as output_file:
				output_file.write(data)
		except IOError, e:
			self.options.parser.error('Output "%s" is not writable: %s' % (output_path, e.strerror))
		
		return True

class CASCDataIndexFile(object):
	def __init__(self, options, index, version, file):
		self.index = index
		self.version = version
		self.file = file
		self.options = options
	
	def open(self):
		if not os.access(self.file, os.R_OK):
			self.options.parser.error('Unable to read index file %s' % self.file)
			return False
		
		fsize = os.stat(self.file).st_size
		with open(self.file, 'rb') as f:
			hdr2_len = struct.unpack('I', f.read(4))[0]
			hdr2_sum = f.read(4) # Unused for now
			
			padding = (8 + hdr2_len + 0x0F) & 0xFFFFFFF0
			# Seek up to start of hdr2
			f.seek(padding, os.SEEK_SET)
			
			data_len, data_sum = struct.unpack('II', f.read(8))
			for entry_idx in xrange(0, data_len / 18):
				key = f.read(9)
				high_byte = struct.unpack('B', f.read(1))[0]
				low_bits = struct.unpack('>I', f.read(4))[0]
				file_size = struct.unpack('I', f.read(4))[0]
				
				data_file_number = (high_byte << 2) | ((low_bits & 0xC0000000) >> 30)
				data_file_offset = (low_bits & 0x3FFFFFFF)

				self.index.AddIndex(key, int(data_file_number), int(data_file_offset), file_size, self.file)
				
				#print key.encode('hex'), data_file_number, data_file_offset, high_byte, low_bits, file_size

		return True

class CASCDataIndex(object):
	def __init__(self, options):
		self.options = options
		self.idx_files = {}
		self.idx_data = {}
	
	def AddIndex(self, key, data_file, data_offset, file_size, indexname):
		if key not in self.idx_data:
			self.idx_data[key] = (data_file, data_offset, file_size)
		else:
			print >>sys.stderr, 'Duplicate key %s in index %s old=%s, new=%s' % (
				key.encode('hex'), indexname, self.idx_data[key], (data_file, data_offset, file_size))
	
	def GetIndexData(self, key):
		return self.idx_data.get(key[:9], (-1, 0, 0))
	
	def open(self):
		data_dir = os.path.join(self.options.data_dir, 'Data', 'data')
		if not os.access(data_dir, os.R_OK):
			self.options.parser.error('Unable to read World of Warcraft data directory %s' % data_dir)
			return False
		
		idx_files_glob = os.path.join(data_dir, '*.idx')
		idx_files = glob.glob(idx_files_glob)
		if len(idx_files) == 0:
			self.options.parser.error('No index files found in World of Warcraft data directory %s' % data_dir)
			return False
		
		idx_file_re = re.compile('([0-9a-fA-F]{2})([0-9a-fA-F]{8})')
		
		selected_idx_files = []
		for idx_file in idx_files:
			mobj = idx_file_re.match(os.path.basename(idx_file))
			if not mobj:
				self.options.parser.error('Unable to match filename %s' % idx_file)
				return False
			
			file_number = int(mobj.group(1), 16)
			file_version = int(mobj.group(2), 16)
			if file_number not in self.idx_files:
				self.idx_files[file_number] = (0, None)
			
			if self.idx_files[file_number][0] < file_version:
				self.idx_files[file_number] = (file_version, idx_file)

		for file_number, file_data in self.idx_files.iteritems():
			self.idx_data[file_number] = CASCDataIndexFile(self.options, self, *file_data)
			if not self.idx_data[file_number].open():
				return False
		
		return True

class CASCEncodingFile(object):
	def __init__(self, options, build):
		self.options = options
		self.build = build
		self.first_entries = []
		self.md5_map = { }
	
	def __bootstrap(self):
		base_path = os.path.dirname(self.options.encoding_file)
		if len(base_path) == 0:
			base_path = os.getcwd()
		
		if not os.access(base_path, os.W_OK):
			self.options.parser.error('Error bootstrapping CASCEncodingFile, "%s" is not writable' % base_path)
		
		try:
			print 'Fetching %s ...' % self.build.encoding_blte_url()
			f = urllib2.urlopen(self.build.encoding_blte_url(), timeout = 5)
			if f.getcode() != 200:
				self.options.parser.error('HTTP request for %s returns %u' % (self.build.encoding_blte_url(), f.getcode()))
		except urllib2.URLError, e:
			self.options.parser.error('Unable to fetch %s: %s' % (self.build.encoding_blte_url(), e.reason))
		
		blte = BLTEFile(f.read())
		if not blte.extract():
			self.options.parser.error('Unable to uncompress BLTE data for encoding file')
	
		md5s = md5.new(blte.output_data).hexdigest()
		if md5s != self.build.encoding_file():
			self.options.parser.error('Invalid md5sum in encoding file, expected %s got %s' % (self.build.encoding_file(), md5s))
	
		with open(self.options.encoding_file, 'wb') as f:
			f.write(blte.output_data)
			
		return blte.output_data
	
	def HasMD5(self, md5s):
		return self.md5_map.has_key(md5s)
	
	def GetFileKeys(self, md5s):
		return self.md5_map.get(md5s, (0, []))[1]
	
	def open(self):
		data = ''
		
		if not os.access(self.options.encoding_file, os.R_OK):
			data = self.__bootstrap()
		else:
			with open(self.options.encoding_file, 'rb') as encoding_file:
				data = encoding_file.read()
		
			md5str = md5.new(data).hexdigest()
			if md5str != self.build.encoding_file():
				data = self.__bootstrap()
		
		offset = 0
		fsize = os.stat(self.options.encoding_file).st_size
		locale = data[offset:offset + 2]; offset += 2
		if locale != 'EN':
			self.options.parser.error('Unknown locale "%s" in file %s, only EN supported' % (locale, self.options.encoding_file))
			return False

		unk_b1, unk_b2, unk_b3, unk_s1, unk_s2, hash_table_size, unk_w1, unk_b3, hash_table_offset = struct.unpack('>BBBHHIIBI', data[offset:offset + 20])
		offset += 20 + hash_table_offset
		
		for hash_entry in xrange(0, hash_table_size):
			md5_file = data[offset:offset + 16]; offset += 16
			md5_sum = data[offset:offset + 16]; offset += 16
			self.first_entries.append(md5_file)

		for hash_block in xrange(0, hash_table_size):
			before = offset
			entry_id = 0
			#print 'Block %u begins at %u' % (hash_block, f.tell())
			n_keys = struct.unpack('H', data[offset:offset + 2])[0]; offset += 2
			while n_keys != 0:
				keys = []
				file_size = struct.unpack('>I', data[offset:offset + 4])[0]; offset += 4
					
				file_md5 = data[offset:offset + 16]; offset += 16
				for key_idx in xrange(0, n_keys):
					keys.append(data[offset:offset + 16]); offset += 16
				
				if entry_id == 0 and file_md5 != self.first_entries[hash_block]:
					self.options.parser.error('Invalid first md5 in block %d@%u, expected %s got %s' % (
						hash_block, offset, self.first_entries[hash_block].encode('hex'), file_md5.encode('hex')))
					return False
				
				#print '%5u %8u %8u %2u %s %s' % (entry_id, f.tell(), file_size, n_keys, file_md5.encode('hex'), file_key.encode('hex'))
				if file_size > 1000000000:
					self.options.parser.error('Invalid (too large) file size %u in block %u, entry id %u, pos %u' % (file_size, hash_block, entry_id, offset))
					return False
				
				if file_md5 in self.md5_map:
					self.options.parser.error('Duplicate md5 entry %s in block %d@%u' % (
						file_md5.encode('hex'), hash_block, offset))
					return False
				
				self.md5_map[file_md5] = (file_size, keys)
			
				entry_id += 1
				n_keys = struct.unpack('H', data[offset:offset + 2])[0]; offset += 2
			
			# Blocks are padded to 4096 bytes it seems, sanity check that the padding is all zeros, though
			for pad_idx in xrange(0, 4096 - (offset - before)):
				byte = data[offset:offset + 1]; offset += 1
				if byte != '\x00':
					self.options.parser.error('Invalid padding byte %u at the end of block %u, pos %u, expected 0, got %#x' % (pad_idx, hash_block, offset, ord(byte)))
					return False
		
		return True
		
class CASCRootFile(object):
	def __init__(self, options, build, encoding, index):
		self.build = build
		self.index = index
		self.encoding = encoding
		self.options = options
		self.hash_map = {}
	
	def __bootstrap(self):
		base_path = os.path.dirname(self.options.root_file)
		if len(base_path) == 0:
			base_path = os.getcwd()
		
		if not os.access(base_path, os.W_OK):
			self.options.parser.error('Error bootstrapping CASCRootFile, "%s" is not writable' % base_path)
		
		keys = self.encoding.GetFileKeys(self.build.root_file().decode('hex'))
		if len(keys) == 0:
			self.options.parser.error('Could not find root file with mdsum %s from data' % self.build.root_file())
		
		file_locations = []
		for key in keys:
			file_locations.append((key, self.build.root_file().decode('hex')) + self.index.GetIndexData(key))
		
		if len(file_locations) == 0:
			self.options.parser.error('Could not find root file location in data')
		
		if len(file_locations) > 1:
			self.options.parser.error('Multiple (%d) file locations for root file in data' % len(file_locations))

		print 'Extracting root file %s ...' % self.build.root_file()
		blte = BLTEExtract(self.options)
		data = blte.extract_data(*file_locations[0])
	
		with open(self.options.root_file, 'wb') as f:
			f.write(data)
			
		return data
	
	def GetFileMD5(self, file):
		hash_name = file.strip().upper().replace('/', '\\')
		hash = jenkins.hashlittle2(hash_name)
		v = (hash[0] << 32) | hash[1]
		return self.hash_map.get(v, [])
	
	def GetFileHashMD5(self, file_hash):
		return self.hash_map.get(file_hash, [])
		
	def open(self):
		if not os.access(self.options.root_file, os.R_OK):
			data = self.__bootstrap()
		else:
			with open(self.options.root_file, 'rb') as root_file:
				data = root_file.read()
		
			md5str = md5.new(data).hexdigest()
			if md5str != self.build.root_file():
				data = self.__bootstrap()
	
		offset = 0
		while offset < len(data):
			n_entries, unk_1, unk_2 = struct.unpack('III', data[offset:offset + 12])
			offset += 12
			if n_entries == 0:
				continue
			
			offset += 4 * n_entries

			for entry_idx in xrange(0, n_entries):
				md5s = data[offset:offset + 16]
				offset += 16
				file_name_hash = data[offset:offset + 8]
				offset += 8
				val = struct.unpack('Q', file_name_hash)[0]
				
				if not val in self.hash_map:
					self.hash_map[val] = []
				
				#print unk_data[entry_idx], file_name_hash.encode('hex'), md5s.encode('hex')
				self.hash_map[val].append(md5s)

		return True
