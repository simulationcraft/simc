# CASC file formats, based on the work of Caali et al. @ http://www.ownedcore.com/forums/world-of-warcraft/world-of-warcraft-model-editing/471104-analysis-of-casc-filesystem.html
import os, sys, mmap, md5, stat, struct, zlib, glob, re

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
		self.extractor = extractor
		self.chunks = []
		self.output_data = ''
		self.extract_status = True
	
	def add_chunk(self, length, c_length, md5s):
		self.chunks.append(BLTEChunk(len(self.chunks), length, c_length, md5s))
	
	def __extract_direct(self):
		type = ord(self.extractor.fd.read(_MARKER_LEN))
		if type != _COMPRESSED_CHUNK:
			self.options.parser.error('Direct extraction only supports compressed data, was given %#x' % type)
			return False
		
		# We don't know where the compressed data ends, so read until it's done
		compressed_data = self.extractor.fd.read(_BLOCK_DATA_SIZE)
		dc = zlib.decompressobj()
		while len(compressed_data) > 0:
			self.output_data += dc.decompress(compressed_data)
			if len(dc.unused_data) > 0:
				# Compressed segment ended at some point, rewind file position
				self.extractor.fd.seek(-len(dc.unused_data), os.SEEK_CUR)
				break
			
			# More data ..
			compressed_data = self.extractor.fd.read(_BLOCK_DATA_SIZE)
		
		return True

	def extract(self):
		pos = self.extractor.fd.tell()
		chunk_data_offset = struct.unpack('>I', self.extractor.fd.read(_CHUNK_DATA_OFFSET_LEN))[0]
		if chunk_data_offset == 0:
			if not self.__extract_direct():
				return False
		else:
			unk_1, cc_b1, cc_b2, cc_b3 = struct.unpack('BBBB', self.extractor.fd.read(_CHUNK_HEADER_LEN))
			if unk_1 != 0x0F:
				self.options.parser.error('Unknown magic byte %d %#x in BLTE @%#.8x' % (unk_1, self.extractor.fd.tell() - _CHUNK_HEADER_LEN))
				return False
			
			n_chunks = (cc_b1 << 16) | (cc_b2 << 8) | cc_b3
			if n_chunks == 0:
				return False
			
			# Chunk information
			for chunk_id in xrange(0, n_chunks):
				c_len, out_len = struct.unpack('>II', self.extractor.fd.read(_CHUNK_HEADER_2_LEN))
				chunk_sum = self.extractor.fd.read(_CHUNK_SUM_LEN)

				self.add_chunk(c_len, out_len, chunk_sum)
			
			# Read chunk data
			for chunk_id in xrange(0, n_chunks):
				chunk = self.chunks[chunk_id]

				data = self.extractor.fd.read(chunk.chunk_length)
				if not chunk.extract(data):
					self.extract_status = False

				self.output_data += chunk.output_data

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
	
	def extract(self, file_name):
		pass
	
	def extract_file(self, file_key, file_md5sum, file_output, data_file_number, data_file_offset, blte_file_size):
		path = os.path.join(self.options.data_dir, 'Data', 'data', 'data.%03u' % data_file_number)
		output_path = os.path.join(self.options.output, file_output)
		
		if not self.open(path):
			return False
		
		if not os.access(os.path.dirname(output_path), os.W_OK):
			self.options.parser.error('Output file %s is not writeable' % output_path)

		self.fd.seek(data_file_offset, os.SEEK_SET)
		
		key = self.fd.read(16)
		blte_len = struct.unpack('<I', self.fd.read(4))[0]
		if key[::-1] != file_key:
			self.options.parser.error('Invalid file key for %s, expected %s, got %s' % (file_output, file_key.encode('hex'), key.encode('hex')))
		
		if blte_len != blte_file_size:
			self.options.parser.error('Invalid file length, expected %u got %u' % (blte_file_size, blte_len))
		
		# Skip 10 bytes of unknown data
		self.fd.seek(10, os.SEEK_CUR)
		if self.fd.read(4) != _BLTE_MAGIC:
			self.options.parser.error('Invalid BLTE magic in file')
				
		file = BLTEFile(self)
		if not file.extract():
			return False
		
		file_md5 = md5.new(file.output_data).digest()
		if file_md5sum != file_md5:
			self.options.parser.error('Invalid md5sum for extracted file, expected %s got %s' % (file_md5sum.encode('hex'), file_md5.encode('hex')))
		
		with open(output_path, 'wb') as output_file:
			output_file.write(file.output_data)

		return True
	
	def extract_file_by_md5(self, file_md5s):
		found = 0
		
		globbed_files = []
		for data_file in self.data_files:
			globbed_files += glob.glob(data_file)
		
		for data_file in globbed_files:
			print 'Checking file %s' % data_file
			if not self.open(data_file):
				return False
			
			while self.fd.tell() < self.fsize:
				# Find next BLTE magic
				next_blte = self.fd.find(_BLTE_MAGIC)
				if next_blte == -1:
					break
				else:
					self.fd.seek(next_blte)
					self.fd.read(len(_BLTE_MAGIC))
				
				blte = BLTEFile(self)
				if not blte.extract():
					return False
				
				output_md5 = md5.new(blte.output_data).hexdigest()
				if output_md5 in file_md5s:
					print 'Found file md5 %s, length=%u' % (output_md5, len(blte.output_data))
					ff = open(output_md5, 'wb')
					ff.write(blte.output_data)
					ff.close()
					found += 1
				
				#elif blte.output_data[:2] == 'EN':
				#	ff = open('encoding_' + output_md5, 'w')
				#	ff.write(blte.output_data)
				#	ff.close()
				#	print 'File matches "EN" at start.'
				
				if found == len(file_md5s):
					break
			
			if found == len(file_md5s):
				break


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

				if not self.index.AddIndex(key, int(data_file_number), int(data_file_offset), file_size):
					return False
				
				#print key.encode('hex'), data_file_number, data_file_offset, high_byte, low_bits, file_size

		return True

class CASCDataIndex(object):
	def __init__(self, options):
		self.options = options
		self.idx_files = {}
		self.idx_data = {}
	
	def AddIndex(self, key, data_file, data_offset, file_size):
		if key in self.idx_data:
			entry = self.idx_data[key]
			print >>sys.stderr, 'Duplicate key %s: old={ file#=%u, file_offset=%u, file_size=%u } new={ file#=%u, file_offset=%u, file_size=%u }' % (
				key.encode('hex'), entry[0], entry[1], entry[2], data_file, data_offset, file_size)
		
		self.idx_data[key] = (data_file, data_offset, file_size)
		return True
	
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
	
	def HasMD5(self, md5s):
		return self.md5_map.has_key(md5s)
	
	def GetFileKeys(self, md5s):
		return self.md5_map.get(md5s, (0, []))[1]
	
	def open(self):
		if not os.access(self.options.encoding_file, os.R_OK):
			self.options.parser.error('Unable to open encoding file "%s" for reading' % self.options.encoding_file)
			return False
		
		with open(self.options.encoding_file, 'rb') as f:
			fsize = os.stat(self.options.encoding_file).st_size
			while f.tell() < fsize:
				locale = f.read(2)
				if locale != 'EN':
					self.options.parser.error('Unknown locale "%s" in file %s, only EN supported' % (locale, self.options.encoding_file))
					return False

				unk_b1, unk_b2, unk_b3, unk_s1, unk_s2, hash_table_size, unk_w1, unk_b3, hash_table_offset = struct.unpack('>BBBHHIIBI', f.read(20))
				#print hash_table_size, hash_table_offset, unk_b1, unk_b2, unk_b3, unk_s1, unk_s2, unk_w1, unk_b3
				f.seek(hash_table_offset, os.SEEK_CUR)
				
				for hash_entry in xrange(0, hash_table_size):
					md5_file = f.read(16)
					md5_sum = f.read(16)
					self.first_entries.append(md5_file)

				for hash_block in xrange(0, hash_table_size):
					before = f.tell()
					entry_id = 0
					#print 'Block %u begins at %u' % (hash_block, f.tell())
					n_keys = struct.unpack('H', f.read(2))[0]
					while n_keys != 0:
						keys = []
						file_size = struct.unpack('>I', f.read(4))[0]
							
						file_md5 = f.read(16)
						for key_idx in xrange(0, n_keys):
							keys.append(f.read(16))
						
						if entry_id == 0 and file_md5 != self.first_entries[hash_block]:
							self.options.parser.error('Invalid first md5 in block %d@%u, expected %s got %s' % (
								hash_block, f.tell(), self.first_entries[hash_block].encode('hex'), file_md5.encode('hex')))
							return False
						
						#print '%5u %8u %8u %2u %s %s' % (entry_id, f.tell(), file_size, n_keys, file_md5.encode('hex'), file_key.encode('hex'))
						if file_size > 1000000000:
							self.options.parser.error('Invalid (too large) file size %u in block %u, entry id %u, pos %u' % (file_size, hash_block, entry_id, f.tell()))
							return False
						
						if file_md5 in self.md5_map:
							self.options.parser.error('Duplicate md5 entry %s in block %d@%u' % (
								file_md5.encode('hex'), hash_block, f.tell()))
							return False
						
						self.md5_map[file_md5] = (file_size, keys)
					
						entry_id += 1
						n_keys = struct.unpack('H', f.read(2))[0]
					
					# Blocks are padded to 4096 bytes it seems, sanity check that the padding is all zeros, though
					for pad_idx in xrange(0, 4096 - (f.tell() - before)):
						byte = f.read(1)
						if byte != '\x00':
							self.options.parser.error('Invalid padding byte %u at the end of block %u, pos %u, expected 0, got %#x' % (pad_idx, hash_block, f.tell(), ord(byte)))
							return False
				break
		
		return True
		
class CASCRootFile(object):
	def __init__(self, options, build):
		self.build = build
		self.options = options
		self.hash_map = {}
	
	def GetFileMD5(self, file):
		hash_name = file.strip().upper().replace('/', '\\')
		hash = jenkins.hashlittle2(hash_name)
		v = (hash[0] << 32) | hash[1]
		return self.hash_map.get(v, [])
	
	def GetFileHashMD5(self, file_hash):
		return self.hash_map.get(file_hash, [])
		
	def open(self):
		if not os.access(self.options.root_file, os.R_OK):
			self.options.parser.error('Unable to open root file %s for reading' % self.options.root_file)
			return False
	
		with open(self.options.root_file, 'rb') as f:
			fsize = os.stat(self.options.root_file).st_size
			while f.tell() < fsize:
				n_entries, unk_1, unk_2 = struct.unpack('III', f.read(12))
				if n_entries == 0:
					continue
				
				f.seek(4 * n_entries, os.SEEK_CUR)

				for entry_idx in xrange(0, n_entries):
					md5s = f.read(16)
					file_name_hash = f.read(8)
					val = struct.unpack('Q', file_name_hash)[0]
					
					if not val in self.hash_map:
						self.hash_map[val] = []
					
					#print unk_data[entry_idx], file_name_hash.encode('hex'), md5s.encode('hex')
					self.hash_map[val].append(md5s)

		return True
