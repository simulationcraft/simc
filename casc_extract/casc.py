# vim: tabstop=4 shiftwidth=4 softtabstop=4
# CASC file formats, based on the work of Caali et al. @ http://www.ownedcore.com/forums/world-of-warcraft/world-of-warcraft-model-editing/471104-analysis-of-casc-filesystem.html
import os, sys, mmap, hashlib, stat, struct, zlib, glob, re, urllib.request, urllib.error, collections, codecs, io, binascii, socket, time

import jenkins

import keyfile

try:
	import salsa20
	no_decrypt = False
except Exception as error:
	print('WARN: %s, salsa20 decryption disabled. Install the Python fixedint (https://pypi.org/project/fixedint/) package to enable' % error,
		file = sys.stderr)
	no_decrypt = True

try:
    import requests
except Exception as error:
    print('ERROR: %s, casc_extract.py requires the Python requests (http://docs.python-requests.org/en/master/) package to function' % error,
		file = sys.stderr)
    sys.exit(1)

_S = requests.Session()
_S.mount('http://', requests.adapters.HTTPAdapter(pool_connections = 5))

_BLTE_MAGIC = b'BLTE'
_CHUNK_DATA_OFFSET_LEN = 4
_CHUNK_HEADER_LEN = 4
_CHUNK_HEADER_2_LEN = 8
_CHUNK_SUM_LEN = 16
_MARKER_LEN = 1
_BLOCK_DATA_SIZE = 65535

_NULL_CHUNK = 0x00
_COMPRESSED_CHUNK = 0x5A
_UNCOMPRESSED_CHUNK = 0x4E
_ENCRYPTED_CHUNK = 0x45

_ROOT_MAGIC = b'TSFM'
_ROOT_HEADER = struct.Struct('<4sII')

_ENCRYPTION_HEADER = struct.Struct('<B8sBIc')

CDNIndexRecord = collections.namedtuple( 'CDNIndexRecord', [ 'index', 'size', 'offset' ] )

class BLTEChunk(object):
	def __init__(self, id, chunk_length, output_length, md5s):
		self.id = id
		self.chunk_length = chunk_length
		self.output_length = output_length
		self.sum = md5s

		self.output_data = b''

	def extract(self, data):
		if len(data) != self.chunk_length:
			sys.stderr.write('Invalid data length for chunk#%d, expected %u got %u\n' % (
				self.id, self.chunk_length, len(data)))
			return False

		type = data[0]
		if type not in [ _NULL_CHUNK, _COMPRESSED_CHUNK, _UNCOMPRESSED_CHUNK, _ENCRYPTED_CHUNK ]:
			sys.stderr.write('Unknown chunk type %#x for chunk%d length=%d\n' % (type,
				self.id, len(data)))
			return False

		if type != 0x00:
			self.__verify(data)

		return self.__process(data)

	def __process(self, data, recursive = False):
		type = data[0]
		if type == _NULL_CHUNK:
			self.output_data = b''
		elif type == _UNCOMPRESSED_CHUNK:
			self.output_data = data[1:]
		elif type == _COMPRESSED_CHUNK:
			dc = zlib.decompressobj()
			uncompressed_data = dc.decompress(data[1:])
			if len(dc.unused_data) > 0:
				sys.stderr.write('Unused %d bytes of compressed data in chunk%d' % (
					len(dc.unused_data), self.id))
				return False

			if len(uncompressed_data) != self.output_length:
				sys.stderr.write('Output chunk data length mismatch in chunk %d, expected %d got %d\n' % (
					self.id, self.output_length, len(uncompressed_data)))
				return False

			self.output_data = uncompressed_data
		elif type == _ENCRYPTED_CHUNK:
			if recursive:
				print("ERROR: Encrypted chunk within encrypted chunk not supported", file = sys.stderr)
				self.output_data = b'\x00' * self.output_len
				return False

			# Could not import salsa20, write zeros
			if no_decrypt:
				self.output_data = b'\x00' * self.output_length
				return True

			offset = 1
			key_name_len = struct.unpack_from('<B', data, offset)[0]
			offset += 1
			if key_name_len != 8:
				sys.stderr.write('Only key name lengths of 8 bytes are supported for encrypted chunks, given %d\n' %
					key_name_len)
				return False

			key_name = data[offset:offset + key_name_len]
			offset += key_name_len

			iv_len = struct.unpack_from('<B', data, offset)[0]
			offset += 1
			if iv_len != 4:
				sys.stderr.write('Only initial vector lengths of 4 bytes are supported for encrypted chunks, given %d\n' %
					iv_len)
				return False

			iv = data[offset:offset + iv_len]
			offset += iv_len

			normalized_iv = b''

			for i in range(0, len(iv)):
				b = iv[i]
				val = (self.id >> (i * 8)) & 0xff
				normalized_iv += (b ^ val).to_bytes(1, byteorder = 'little')

			type_ = struct.unpack_from('<c', data, offset)[0]
			offset += 1

			if type_ != b'S':
				sys.stderr.write('Only salsa20 encryption supported, given "%s"\n' %
					type_.decode('ascii'))
				return False

			#sys.stderr.write('Encrypted chunk %d, type=%s, key_name_len=%d, key_name=%s, iv_len=%d iv=%s (%s), sz=%d, c_len=%d, o_len=%d offset=%d\n' % (
			#	self.id, type_.decode('ascii'), key_name_len, binascii.hexlify(key_name).decode('ascii'), iv_len,
			#	binascii.hexlify(iv).decode('ascii'), binascii.hexlify(normalized_iv).decode('ascii'),
			#	len(data) - offset, self.chunk_length, self.output_length, offset
			#))

			key = keyfile.find(key_name)
			# No encryption key in the database, just write zeros
			if not key:
				self.output_data = b'\x00' * self.output_length
				return True

			state = salsa20.initialize(key, normalized_iv)
			tmp_data = salsa20.decrypt(state, data[offset:])

			# Run chunk processing once more, since the decrypted data is now a valid
			# chunk to perform (a non-decryption) BLTE operation on
			return self.__process(tmp_data, True)
		else:
			sys.stderr.write('Unknown chunk %d, type=%c, sz=%d, out_len=%d\n' % (
				self.id, data[0], len(data), self.output_length))
			self.output_data = b'\x00' * self.output_length

		return True

	def __verify(self, data):
		md5s = hashlib.md5(data).digest()
		if md5s != self.sum:
			sys.stderr.write('Chunk%d of type %#x fails verification, expects %s got %s\n' % (
				self.id, data[0], codecs.encode(self.sum, 'hex').decode('utf-8'),
				codecs.encode(md5s, 'hex').decode('utf-8')))
			return False

		return True

class BLTEFile(object):
	def __init__(self, extractor):
		self.data = extractor
		self.offset = 0
		self.chunks = []
		self.output_data = b''
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
			for chunk_id in range(0, n_chunks):
				c_len, out_len = struct.unpack('>II', self.__read(_CHUNK_HEADER_2_LEN))
				chunk_sum = self.__read(_CHUNK_SUM_LEN)

				#print('Chunk#%d@%u: c_len=%u out_len=%u sum=%s' % (chunk_id, self.__tell() - 24, c_len, out_len, chunk_sum.encode('hex')))
				self.add_chunk(c_len, out_len, chunk_sum)

			# Read chunk data
			sum_in_file = 0
			for chunk_id in range(0, n_chunks):
				chunk = self.chunks[chunk_id]

				#print('Chunk#%d@%u: Data extract len=%u, total=%u' % (chunk_id, self.__tell(), chunk.chunk_length, sum_in_file))
				data = self.__read(chunk.chunk_length)
				if not chunk.extract(data):
					self.extract_status = False
					return False

				self.output_data += chunk.output_data
				sum_in_file += len(chunk.output_data)
				#print(chunk_id, sum_in_file)

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

	def extract_buffer_to_file(self, data, fname):
		dirname = os.path.dirname(os.path.normpath(fname))
		if not os.path.exists(dirname):
			os.makedirs(dirname)

		data = self.extract_buffer(data)
		if not data:
			print('Unable to extract %s ...' % os.path.basename(fname))
			return

		with open(fname, 'wb') as f:
			f.write(data)

	def extract_buffer(self, data):
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
			self.options.parser.error('Invalid file key for %s, expected %s, got %s' % (
				file_output, codecs.encode(file_key, 'hex').decode('utf-8'), codecs.encode(key, 'hex').decode('utf-8')))

		if blte_len != blte_file_size:
			self.options.parser.error('Invalid file length, expected %u got %u' % (blte_file_size, blte_len))

		# Skip 10 bytes of unknown data
		self.fd.seek(10, os.SEEK_CUR)

		file = self.__extract_file()
		if not file:
			return None

		file_md5 = hashlib.md5(file.output_data).digest()
		if file_md5sum and file_md5sum != file_md5:
			self.options.parser.error('Invalid md5sum for extracted file, expected %s got %s' % (
				codecs.encode(file_md5sum, 'hex').decode('utf-8'), codecs.encode(file_md5, 'hex').decode('utf-8')))

		self.close()

		return file.output_data

	def extract_file(self, file_key, file_md5sum, file_output, data_file_number, data_file_offset, blte_file_size):
		output_path = ''
		if file_output:
			output_path = os.path.join(self.options.output, file_output)
		else:
			if file_key:
				output_path = os.path.join(self.options.output, codecs.encode(file_key, 'hex').decode('utf-8'))
			elif file_md5sum:
				output_path = os.path.join(self.options.output, codecs.encode(file_md5sum, 'hex').decode('utf-8'))

		output_dir = os.path.dirname(os.path.abspath(output_path))
		try:
			if not os.path.exists(output_dir):
				os.makedirs(output_dir)
		except os.error as e:
			self.options.parser.error('Output "%s" is not writable: %s' % (output_path, e.strerror))

		data = self.extract_data(file_key, file_md5sum, data_file_number, data_file_offset, blte_file_size)
		if not data:
			return False

		try:
			with open(output_path, 'wb') as output_file:
				output_file.write(data)
		except IOError as e:
			self.options.parser.error('Output "%s" is not writable: %s' % (output_path, e.strerror))

		return True

class CASCObject(object):
	def __init__(self, options):
		self.options = options

	def get_url(self, url, headers = None):
		attempt = 0
		maxAttempts = 5
		while attempt < maxAttempts:
			r = None
			try:
				r = _S.get(url, headers = headers)

				print('Fetching %s ...' % url)
				if r.status_code not in [200, 206]:
					self.options.parser.error('HTTP request for %s returns %u' % (url, r.status_code))
				return r
			except Exception as e:
				self.options.parser.error('Unable to fetch %s: %s' % (url, r.reason if r else 'unknown error'))
				if attempt + 1 < maxAttempts:
					print('Retrying...')
					time.sleep(2 ** attempt)
					attempt += 1


	def cache_dir(self, path = None):
		dir = self.options.cache
		if path:
			dir = os.path.join(self.options.cache, path)

		if not os.path.exists(dir):
			try:
				os.makedirs(dir)
			except os.error as e:
				self.options.parser('Unable to make %s: %s' % (dir, e.strerror))

		return dir

	def cached_open(self, file, url, headers = None):
		handle = None

		if not os.path.exists(file):
			handle = self.get_url(url, headers)
			if handle.status_code < 200 and handle.status_code > 299:
				return None

			data = handle.content
			with open(file, 'wb') as f:
				f.write(data)

			handle = io.BytesIO(data)
		else:
			handle = open(file, 'rb')

		return handle

class BuildCfg:
	def __init__(self, handle):
		self.handle = handle

		for line in self.handle:
			mobj = re.match('^([^ ]+)[ ]*=[^A-z0-9]*(.+)', line.decode('utf-8'))
			if not mobj:
				continue

			data = mobj.group(2)
			if ' ' in data:
				data = data.split(' ')

			key = mobj.group(1).replace('-', '_')

			setattr(self, key, data)

class CDNIndex(CASCObject):
	PATCH_BASE_URL = 'http://us.patch.battle.net:1119'

	PATCH_BETA = 'wow_beta'
	PATCH_ALPHA = 'wow_alpha'
	PATCH_PTR = 'wowt'
	PATCH_LIVE = 'wow'
	PATCH_CLASSIC = 'wow_classic'

	def __init__(self, options):
		CASCObject.__init__(self, options)

		self.cdn_hash = None
		self.cdn_host = None
		self.cdn_path = None
		self.build_cfg_hash = []
		self.archives = []
		self.cdn_index = {}
		self.builds = []

		self.version = None
		self.build_number = 0
		self.build_version = 0
		self.cdn_idx = 0
		self.bgdl = options.bgdl

	# TODO: (More) Option based selectors
	def get_build_cfg(self):
		return self.builds[0]

	def build(self):
		return self.version

	def root_file(self):
		return self.get_build_cfg().root

	def encoding_file(self):
		return self.get_build_cfg().encoding[0]

	def encoding_blte_url(self):
		return self.cdn_url('data', self.get_build_cfg().encoding[1])

	def patch_base_url(self):
		if self.options.product:
			return '%s/%s' % ( CDNIndex.PATCH_BASE_URL, self.options.product )
		elif self.options.ptr:
			return '%s/%s' % ( CDNIndex.PATCH_BASE_URL, CDNIndex.PATCH_PTR )
		elif self.options.beta:
			return '%s/%s' % ( CDNIndex.PATCH_BASE_URL, CDNIndex.PATCH_BETA )
		elif self.options.alpha:
			return '%s/%s' % ( CDNIndex.PATCH_BASE_URL, CDNIndex.PATCH_ALPHA )
		elif self.options.classic:
			return '%s/%s' % ( CDNIndex.PATCH_BASE_URL, CDNIndex.PATCH_CLASSIC )
		else:
			return '%s/%s' % ( CDNIndex.PATCH_BASE_URL, CDNIndex.PATCH_LIVE )

	def open_cdns(self):
		cdns_url = '%s/cdns' % self.patch_base_url()
		handle = self.get_url(cdns_url)

		data = handle.text.strip().split('\n')
		for line in data:
			split = line.split('|')
			if split[0] != self.options.region:
				continue

			self.cdn_path = split[1]
			#cdns = [urllib.parse.urlparse(x).netloc for x in split[3].split(' ')]
			#self.cdn_host = cdns[-1]
			self.cdn_host = split[2].split(' ')

		if not self.cdn_path or not self.cdn_host:
			sys.stderr.write('Unable to extract CDN information\n')
			sys.exit(1)

	def cdn_base_url(self):
		s = 'http://%s/%s' % (self.cdn_host[self.cdn_idx], self.cdn_path)

		self.cdn_idx += 1
		if self.cdn_idx == len(self.cdn_host):
			self.cdn_idx = 0

		return s

	def cdn_url(self, type, file):
		return '%s/%s/%s/%s/%s' % (self.cdn_base_url(), type, file[:2], file[2:4], file)

	def open_version(self):
		version_url =  '%s/%s' % (self.patch_base_url(), 'bgdl' if self.bgdl else 'versions')
		handle = self.get_url(version_url)

		for line in handle.iter_lines():
			split = line.decode('utf-8').strip().split('|')
			if split[0] != 'us':
				continue

			if len(split) != 7:
				sys.stderr.write('Version format mismatch, expected 7 fields, got %d\n' % len(split))
				sys.exit(1)

			# The CDN hash name is what we want at this point
			self.cdn_hash = split[2]
			self.version = split[5]
			self.build_number = int(split[4])

			version_split = self.version.split('.')
			if len(version_split) == 3:
				self.build_version = split[6]
			# Yank out the build number from the version
			elif len(version_split) == 4:
				self.build_version = '.'.join(version_split[0:-1])
			else:
				sys.stderr.write('Unable to parse version from "%s" data\n' % self.version)
				sys.exit(1)

			# Also take build configuration information from the versions file
			# nowadays, as the "CDN" file builds option may have things like
			# background downloader builds in it
			self.build_cfg_hash = [ split[1], ]

		if not self.cdn_hash:
			sys.stderr.write('Invalid version file\n')
			sys.exit(1)

		print('Current build version: %s [%d]' % (self.version, self.build_number))

	def open_cdn_build_cfg(self):
		path = os.path.join(self.cache_dir('config'), self.cdn_hash)
		url = self.cdn_url('config', self.cdn_hash)

		handle = self.cached_open(path, url)
		if not handle:
			self.options.parser.error('Unable to fetch CDN configuration')

		for line in handle:
			mobj = re.match('^archives = (.+)', line.decode('utf-8'))
			if mobj:
				self.archives = mobj.group(1).split(' ')
				continue

			mobj = re.match('^builds = (.+)', line.decode('utf-8'))
			if mobj:
				# Always take the first build configuration
				self.build_cfg_hash = mobj.group(1).split(' ')
				continue

	def open_build_cfg(self):
		for cfg in self.build_cfg_hash:
			path = os.path.join(self.cache_dir('config'), cfg)
			url = self.cdn_url('config', cfg)
			handle = self.cached_open(path, url)
			if not handle:
				self.options.parser.error('Unable to fetch build configuration')

			self.builds.append(BuildCfg(handle))

	def open_archives(self):
		sys.stdout.write('Parsing CDN index files ... ')

		index_cache = self.cache_dir('index')
		for idx in range(0, len(self.archives)):
			index_file_name = '%s.index' % self.archives[idx]
			index_file_path = os.path.join(index_cache, index_file_name)
			index_file_url = self.cdn_url('data', index_file_name)

			handle = self.cached_open(index_file_path, index_file_url)
			if not self.parse_archive(handle, idx):
				self.options.parser.error('Unable to parse index file %s, aborting ...' % index_file_name)

		sys.stdout.write('%u entries\n' % len(self.cdn_index.keys()))

	def parse_archive(self, handle, idx):
		handle.seek(-12, os.SEEK_END)
		record_count = struct.unpack('<i', handle.read(4))[0]
		if record_count == 0:
			return False

		handle.seek(0, os.SEEK_SET)
		for record_idx in range(0, record_count):
			key = handle.read(16)
			size, offset = struct.unpack('>ii', handle.read(8))

			if key in self.cdn_index:
				sys.stderr.write('Key %s, %d, %d, %d exists in index @ (%s, %d, %d, %d)\n' % (
					codecs.encode(key, 'hex').decode('utf-8'), idx, size, offset,
					self.archives[self.cdn_index[key].index],
					self.cdn_index[key].size, self.cdn_index[ key ].offset ))

			self.cdn_index[key] = CDNIndexRecord(idx, size, offset)

			block_bytes_left = 4096 - handle.tell() % 4096
			if block_bytes_left < 24:
				handle.read(block_bytes_left)

		return True

	def CheckVersion(self):
		self.open_version()
		self.open_cdns()
		self.open_cdn_build_cfg()
		self.open_build_cfg()

		return True

	def open(self):
		self.open_version()
		self.open_cdns()
		self.open_cdn_build_cfg()
		self.open_build_cfg()
		self.open_archives()

		return True

	def fetch_file(self, key):
		key_info = self.cdn_index.get(key, None)
		handle = None
		key_file_path = os.path.join(self.cache_dir('data'), codecs.encode(key, 'hex').decode('utf-8'))
		if key_info:
			key_file_url = self.cdn_url('data', self.archives[key_info.index])

			handle = self.cached_open(key_file_path, key_file_url, {'Range': 'bytes=%d-%d' % (key_info.offset, key_info.offset + key_info.size - 1)})
		else:
			handle = self.cached_open(key_file_path, self.cdn_url('data', codecs.encode(key, 'hex').decode('utf-8')))

		if not handle:
			return None

		return handle.read()

class RibbitIndex(CDNIndex):
	def get_url(self, content, headers = None):
		if headers or 'http://' in content:
			return super().get_url(content, headers)
		else:
			s = socket.create_connection(('us.version.battle.net', 1119), 10)
			n_sent = s.send(bytes(content + '\r\n', 'ascii'))
			return s.makefile('b')

	def patch_base_url(self):
		if self.options.product:
			return 'v1/products/%s' % self.options.product
		elif self.options.ptr:
			return 'v1/products/%s' % self.PATCH_PTR
		elif self.options.beta:
			return 'v1/products/%s' % self.PATCH_BETA
		elif self.options.alpha:
			return 'v1/products/%s' % self.PATCH_ALPHA
		elif self.options.classic:
			return 'v1/products/%s' % self.PATCH_CLASSIC
		else:
			return 'v1/products/%s' % self.PATCH_LIVE

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
			for entry_idx in range(0, data_len // 18):
				key = f.read(9)
				high_byte = struct.unpack('B', f.read(1))[0]
				low_bits = struct.unpack('>I', f.read(4))[0]
				file_size = struct.unpack('I', f.read(4))[0]

				data_file_number = (high_byte << 2) | ((low_bits & 0xC0000000) >> 30)
				data_file_offset = (low_bits & 0x3FFFFFFF)

				self.index.AddIndex(key, int(data_file_number), int(data_file_offset), file_size, self.file)

				#print(key.encode('hex'), data_file_number, data_file_offset, high_byte, low_bits, file_size)

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
			sys.stderr.write('Duplicate key %s in index %s old=%s, new=%s\n' % (
				codecs.encode(key, 'hex').decode('utf-8'), indexname, self.idx_data[key], (data_file, data_offset, file_size)))

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

		for file_number, file_data in self.idx_files.items():
			self.idx_data[file_number] = CASCDataIndexFile(self.options, self, *file_data)
			if not self.idx_data[file_number].open():
				return False

		return True

class CASCEncodingFile(CASCObject):
	def __init__(self, options, build):
		CASCObject.__init__(self, options)
		self.build = build
		self.first_entries = []
		self.md5_map = { }

	def encoding_path(self):
		base_cached_name = 'encoding'
		if self.options.ptr:
			base_cached_name += '.ptr'
		elif self.options.beta:
			base_cached_name += '.beta'
		elif self.options.alpha:
			base_cached_name += '.alpha'
		elif self.options.classic:
			base_cached_name += '.classic'

		return os.path.join(self.cache_dir(), base_cached_name)

	def __bootstrap(self):
		if not os.access(self.cache_dir(), os.W_OK):
			self.options.parser.error('Error bootstrapping CASCEncodingFile, "%s" is not writable' % self.cache_dir())

		encoding_file_path = os.path.join(self.cache_dir('data'), self.build.encoding_file())
		handle = self.cached_open(encoding_file_path, self.build.encoding_blte_url())
		#handle = self.get_url(self.build.encoding_blte_url())
		if not handle:
			self.options.parser.error('Unable to fetch encoding file')

		blte = BLTEFile(handle.read())
		if not blte.extract():
			self.options.parser.error('Unable to uncompress BLTE data for encoding file')

		md5s = hashlib.md5(blte.output_data).hexdigest()
		if md5s != self.build.encoding_file():
			self.options.parser.error('Invalid md5sum in encoding file, expected %s got %s' % (self.build.encoding_file(), md5s))

		with open(self.encoding_path(), 'wb') as f:
			f.write(blte.output_data)

		return blte.output_data

	def HasMD5(self, md5s):
		return self.md5_map.has_key(md5s)

	def GetFileKeys(self, md5s):
		return self.md5_map.get(md5s, (0, []))[1]

	def open(self):
		data = ''

		if not os.access(self.encoding_path(), os.R_OK):
			data = self.__bootstrap()
		else:
			with open(self.encoding_path(), 'rb') as encoding_file:
				data = encoding_file.read()

			md5str = hashlib.md5(data).hexdigest()
			if md5str != self.build.encoding_file():
				data = self.__bootstrap()

		sys.stdout.write('Parsing encoding file %s ... ' % self.build.encoding_file())

		offset = 0
		fsize = os.stat(self.encoding_path()).st_size
		magic = data[offset:offset + 2]; offset += 2
		if magic != b'EN':
			self.options.parser.error('Unknown magic "%s" in encoding file %s' % (locale, self.encoding_path()))
			return False

		unk_b1, unk_b2, unk_b3, unk_s1, unk_s2, hash_table_size, unk_hash_count, unk_b4, hash_table_offset = struct.unpack('>BBBHHIIBI', data[offset:offset + 20])
		#print(unk_b1, unk_b2, unk_b3, unk_s1, unk_s2, hash_table_size, unk_hash_count, unk_b4, hash_table_offset)
		offset += 20 + hash_table_offset

		for hash_entry in range(0, hash_table_size):
			md5_file = data[offset:offset + 16]; offset += 16
			#md5_sum = data[offset:offset + 16];
			# Skip the second MD5sum, we don't use it for anything
			offset += 16
			self.first_entries.append(md5_file)

		for hash_block in range(0, hash_table_size):
			before = offset
			entry_id = 0
			#print('Block %u begins at %u' % (hash_block, f.tell()))
			n_keys = struct.unpack('H', data[offset:offset + 2])[0]; offset += 2
			while n_keys != 0:
				keys = []
				file_size = struct.unpack('>I', data[offset:offset + 4])[0]; offset += 4
				file_md5 = data[offset:offset + 16]; offset += 16
				for key_idx in range(0, n_keys):
					keys.append(data[offset:offset + 16]); offset += 16

				if entry_id == 0 and file_md5 != self.first_entries[hash_block]:
					self.options.parser.error('Invalid first md5 in block %d@%u, expected %s got %s' % (
						hash_block, offset,
						codecs.encode(self.first_entries[hash_block], 'hex').decode('utf-8'),
						codecs.encode(file_md5, 'hex').decode('utf-8')))
					return False

				#print('%5u %8u %8u %2u %s %s' % (entry_id, f.tell(), file_size, n_keys, file_md5.encode('hex'), file_key.encode('hex')))
				if file_size > 1000000000:
					self.options.parser.error('Invalid (too large) file size %u in block %u, entry id %u, pos %u' % (file_size, hash_block, entry_id, offset))
					return False

				if file_md5 in self.md5_map:
					self.options.parser.error('Duplicate md5 entry %s in block %d@%u' % (
						codecs.encode(file_md5, 'hex').decode('utf-8'), hash_block, offset))
					return False

				self.md5_map[file_md5] = (file_size, keys)

				entry_id += 1
				n_keys = struct.unpack('H', data[offset:offset + 2])[0]; offset += 2

			# Blocks are padded to 4096 bytes it seems, sanity check that the padding is all zeros, though
			for pad_idx in range(0, 4096 - (offset - before)):
				byte = data[offset:offset + 1]; offset += 1
				if byte != b'\x00':
					self.options.parser.error('Invalid padding byte %u at the end of block %u, pos %u, expected 0, got %#x' % (pad_idx, hash_block, offset, ord(byte)))
					return False

		sys.stdout.write('%u entries\n' % len(self.md5_map.keys()))

		# Rest of the encoding file is unnecessary for now

		#for i in range(0, unk_hash_count):
		#	tmp = offset
		#	hash = data[offset:offset + 16]; offset += 16
		#	hash2 = data[offset:offset + 16]; offset += 16
		#	unk_word1, unk_word2 = struct.unpack('>II', data[offset:offset + 8]); offset += 8
		#	unk_b1 = data[offset]; offset += 1
		#	print(tmp, hash.encode('hex'), hash2.encode('hex'))

		#n = 0
		#start = offset
		#while offset < fsize:
		#	tmp = offset
		#	hash = data[offset:offset + 16]; offset += 16
		#	opt1 = struct.unpack('>IBI', data[offset:offset + 9])
		#	self.key_map[hash] = opt1
		#	offset += 9
		#	if opt1[0] == 0xFFFFFFFF:
		#		break
		#	print(n, tmp, 4096 - (offset - start), offset - start, hash.encode('hex'), opt1)
		#	n += 1
		#	if 4096 - (offset - start) < 25:
		#		offset += 4096 - (offset - start)
		#		start = offset
		#
		#print(offset)

		return True

class CASCRootFile(CASCObject):
	# en_US locale
	_locale = {
		'en_US' : 0x2,
		'ko_KR' : 0x4,
		'fr_FR' : 0x10,
		'de_DE' : 0x20,
		'zh_CN' : 0x40,
		'es_ES' : 0x80,
		'zh_TW' : 0x100,
		'en_GB' : 0x200,
		'en_CN' : 0x400,
		'en_TW' : 0x800,
		'es_MX' : 0x1000,
		'ru_RU' : 0x2000,
		'pt_BR' : 0x4000,
		'it_IT' : 0x8000,
		'pt_PT' : 0x10000
	}

	LOCALE_ALL = 0xFFFFFFFF

	def __init__(self, options, build, encoding, index):
		CASCObject.__init__(self, options)

		self.build = build
		self.index = index
		self.encoding = encoding
		# FileDataID to md5sum map
		self.hash_map = {}
		# FilePathHash to md5sum map
		self.path_hash_map = {}

		if options.locale not in CASCRootFile._locale:
			self.options.parser.error('Invalid locale, valid values are %s' % (', '.join(CASCRootFile._locale.keys())))

		self.locale = CASCRootFile._locale[self.options.locale]

	def root_path(self):
		base_cached_name = 'root'
		if self.options.ptr:
			base_cached_name += '.ptr'
		elif self.options.beta:
			base_cached_name += '.beta'
		elif self.options.alpha:
			base_cached_name += '.alpha'
		elif self.options.classic:
			base_cached_name += '.classic'

		return os.path.join(self.cache_dir(), base_cached_name)

	def __bootstrap(self):
		if not os.access(self.cache_dir(), os.W_OK):
			self.options.parser.error('Error bootstrapping CASCRootFile, "%s" is not writable' % self.cache_dir())

		keys = self.encoding.GetFileKeys(codecs.decode(self.build.root_file(), 'hex'))
		if len(keys) == 0:
			self.options.parser.error('Could not find root file with mdsum %s from data' % self.build.root_file())

		blte = BLTEExtract(self.options)
		# Extract from local files
		if not self.options.online:
			file_locations = []
			for key in keys:
				file_locations.append((key, codecs.decode(self.build.root_file(), 'hex')) + self.index.GetIndexData(key))

			if len(file_locations) == 0:
				self.options.parser.error('Could not find root file location in data')

			if len(file_locations) > 1:
				self.options.parser.error('Multiple (%d) file locations for root file in data' % len(file_locations))

			data = blte.extract_data(*file_locations[0])
		# Fetch from online
		else:
			if len(keys) > 1:
				print('Duplicate root key found for %s, using first one ...' % self.build.root_file())

			root_file_path = os.path.join(self.cache_dir('data'), self.build.root_file())
			handle = self.cached_open(root_file_path,
				self.build.cdn_url('data', codecs.encode(keys[0], 'hex').decode('utf-8')))
			#handle = self.get_url(self.build.cdn_url('data', codecs.encode(keys[0], 'hex').decode('utf-8')))

			if not handle:
				self.options.parser.error('Unable to fetch root file')

			blte = BLTEFile(handle.read())
			if not blte.extract():
				self.options.parser.error('Unable to uncompress BLTE data for root file')

			md5s = hashlib.md5(blte.output_data).hexdigest()
			if md5s != self.build.root_file():
				self.options.parser.error('Invalid md5sum in root file, expected %s got %s' % (self.build.root_file(), md5s))

			data = blte.output_data

		with open(self.root_path(), 'wb') as f:
			f.write(data)

		return data

	def GetFileDataIdMD5(self, file_data_id):
		return self.hash_map.get(file_data_id, [])

	def GetFilePathMD5(self, hash):
		return self.path_hash_map.get(hash, [])

	def GetLocale(self):
		if self.options.locale not in CASCRootFile._locale:
			return 0

		flags = CASCRootFile._locale[self.options.locale]
		if self.options.locale in ['en_US', 'en_GB']:
			flags = CASCRootFile._locale['en_US'] | CASCRootFile._locale['en_GB']

		return flags

	def __parse_default(self, data):
		offset = 0
		n_md5s = 0

		magic, unk_h1, unk_h2 = _ROOT_HEADER.unpack_from(data, offset)

		if magic != _ROOT_MAGIC:
			print('Invalid magic in file, expected "{:s}", got "{:s}"'.format(
				_ROOT_MAGIC.decode('ascii'), magic.decode('ascii')))
			return False
		offset += _ROOT_HEADER.size

		#print(magic, unk_h1, unk_h2)

		while offset < len(data):
			n_entries, flags, locale = struct.unpack_from('<iII', data, offset)
			#print('offset', offset, 'n-entries', n_entries, 'content_flags', '{:#8x}'.format(flags), 'locale', '{:#8x}'.format(locale))
			#if flags == 0xFFFFFFFF or flags & 0x2:
			#	print('%u %d, unk_1=%#.8x, flags=%#.8x' % (offset, n_entries, unk_1, flags))
			offset += 12
			if n_entries == 0:
				continue

			findex = struct.unpack_from('<{}I'.format(n_entries), data, offset)
			offset += 4 * n_entries

			csum_file_id = 0
			file_ids = []
			for entry_idx in range(0, n_entries):
				file_path_hash = None

				md5s = data[offset:offset + 16]
				offset += 16

				file_data_id = 0
				if entry_idx == 0:
					file_data_id = findex[entry_idx]
				else:
					file_data_id = csum_file_id + 1 + findex[entry_idx]
				csum_file_id = file_data_id

				if locale != CASCRootFile.LOCALE_ALL and not (locale & self.GetLocale()):
					file_ids.append(None)
					continue

				if not file_data_id in self.hash_map:
					self.hash_map[file_data_id] = []

				self.hash_map[file_data_id].append(md5s)
				file_ids.append(file_data_id)

				n_md5s += 1

			# If the flags does not include the "no name hashes", or we're
			# parsing a classic root file, the next 8 bytes will be the
			# file path jenkins hash
			if flags & 0x10000000 == 0:
				_PATH_HASH = struct.Struct('<{}Q'.format(n_entries))
				hashes = _PATH_HASH.unpack_from(data, offset)
				for entry_idx in range(0, n_entries):
					if file_ids[entry_idx] == None:
						continue

					if hashes[entry_idx] not in self.path_hash_map:
						self.path_hash_map[hashes[entry_idx]] = []
						self.path_hash_map[hashes[entry_idx]] += self.hash_map[file_data_id]

				#file_path_hash = _PATH_HASH.unpack_from(data, offset)[0]
				offset += _PATH_HASH.size

		sys.stdout.write('%u entries\n' % n_md5s)

		return True

	def __parse_classic(self, data):
		offset = 0
		n_md5s = 0

		_PATH_HASH = struct.Struct('<Q')

		while offset < len(data):
			n_entries, flags, locale = struct.unpack_from('<iII', data, offset)
			#print('offset', offset, 'n-entries', n_entries, 'content_flags', '{:#8x}'.format(flags), 'locale', '{:#8x}'.format(locale))
			#if flags == 0xFFFFFFFF or flags & 0x2:
			#	print('%u %d, unk_1=%#.8x, flags=%#.8x' % (offset, n_entries, unk_1, flags))
			offset += 12
			if n_entries == 0:
				continue

			findex = struct.unpack_from('<{}I'.format(n_entries), data, offset)
			offset += 4 * n_entries

			csum_file_id = 0
			for entry_idx in range(0, n_entries):
				file_path_hash = None

				md5s = data[offset:offset + 16]
				offset += 16

				file_data_id = 0
				if entry_idx == 0:
					file_data_id = findex[entry_idx]
				else:
					file_data_id = csum_file_id + 1 + findex[entry_idx]
				csum_file_id = file_data_id

				hash_ = _PATH_HASH.unpack_from(data, offset)[0]
				offset += _PATH_HASH.size

				if locale != CASCRootFile.LOCALE_ALL and not (locale & self.GetLocale()):
					continue

				if not file_data_id in self.hash_map:
					self.hash_map[file_data_id] = []

				self.hash_map[file_data_id].append(md5s)

				if not hash_ in self.path_hash_map:
					self.path_hash_map[hash_] = []

				self.path_hash_map[hash_].append(md5s)

				n_md5s += 1

		sys.stdout.write('%u entries\n' % n_md5s)
		return True

	def open(self):
		if not os.access(self.root_path(), os.R_OK):
			data = self.__bootstrap()
		else:
			with open(self.root_path(), 'rb') as root_file:
				data = root_file.read()

			md5str = hashlib.md5(data).hexdigest()
			if md5str != self.build.root_file():
				data = self.__bootstrap()

		sys.stdout.write('Parsing root file %s ... ' % self.build.root_file())

		if len(data) < 4:
			sys.stdout.write('Root file too small (%d bytes)' % len(data))
			return False

		if data[:4] == b'TSFM':
			return self.__parse_default(data)
		else:
			return self.__parse_classic(data)

