"""
CASC file formats, based on the work of Caali et al. @
http://www.ownedcore.com/forums/world-of-warcraft/world-of-warcraft-model-editing/471104-analysis-of-casc-filesystem.html
"""
import codecs
import collections
import glob
import hashlib
import io
import math
import mmap
import os
import random
import re
import socket
import struct
import sys
import time
import zlib

import jenkins
import keyfile
import build_cfg

try:
    import salsa20
    NO_DECRYPT = False
except ImportError as error:
    print(
        f"WARN: {error}, salsa20 decryption disabled. Install the Python fixedint "
        f"(https://pypi.org/project/fixedint/) package to enable",
        file=sys.stderr)
    NO_DECRYPT = True

try:
    import requests
except ImportError as error:
    print(
        f"ERROR: {error}, casc_extract.py requires the Python requests "
        f"(http://docs.python-requests.org/en/master/) package to function",
        file=sys.stderr)
    sys.exit(1)

_S = requests.Session()
_S.mount('http://', requests.adapters.HTTPAdapter(pool_connections=5))

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
_ROOT_HEADER_2 = struct.Struct('<III')

_ENCRYPTION_HEADER = struct.Struct('<B8sBIc')

_LOCAL_IDX_HEADER = struct.Struct('<IIHBBBBBBQ8sII')

# Note, hardcodes checksumSize to 8 bytes
_ARCHIVE_IDX_FOOTER = struct.Struct('<8sbbbbbbbbI8s')
_ARCHIVE_IDX_ENTRY = struct.Struct('>16sII')

CDNIndexRecord = collections.namedtuple('CDNIndexRecord',
                                        ['index', 'size', 'offset'])


class BLTEChunk:
    def __init__(self, id_, chunk_length, output_length, md5s):
        self.id = id_
        self.chunk_length = chunk_length
        self.output_length = output_length
        self.sum = md5s
        self.has_encryption = False
        self.is_decrypted = False

        self.output_data = b''

    def decrypted(self):
        return not self.has_encryption or self.is_decrypted

    def extract(self, data):
        if len(data) != self.chunk_length:
            print(f"Invalid data length for chunk#{self.id}, expected {self.chunk_length} "
                  f"got {len(data)}",
                  file=sys.stderr)
            return False

        type_ = data[0]
        if type_ not in [_NULL_CHUNK, _COMPRESSED_CHUNK, _UNCOMPRESSED_CHUNK, _ENCRYPTED_CHUNK]:
            print(f"Unknown chunk type {type_:#x} for chunk{self.id} length={len(data)}",
                  file=sys.stderr)
            return False

        if type != 0x00:
            self.__verify(data)

        return self.__process(data)

    def __process_compressed(self, data):
        dc = zlib.decompressobj()
        uncompressed_data = dc.decompress(data[1:])
        if len(dc.unused_data) > 0:
            print(
                f"Unused {len(dc.unused_data)} bytes of compressed data in chunk{self.id}",
                file=sys.stderr)
            return False

        if len(uncompressed_data) != self.output_length:
            print(
                f"Output chunk data length mismatch in chunk {self.id}, expected "
                f"{self.output_length} got {len(uncompressed_data)}",
                file=sys.stderr)
            return False

        self.output_data = uncompressed_data

        return True

    def __process_encrypted(self, data):
        self.has_encryption = True

        # Could not import salsa20, write zeros
        if NO_DECRYPT:
            self.output_data = b'\x00' * self.output_length
            return True

        offset = 1
        key_name_len = struct.unpack_from('<B', data, offset)[0]
        offset += 1
        if key_name_len != 8:
            print(
                f"Only key name lengths of 8 bytes are supported for encrypted chunks, "
                f"given {key_name_len}",
                file=sys.stderr)
            return False

        key_name = data[offset:offset + key_name_len]
        offset += key_name_len

        iv_len = struct.unpack_from('<B', data, offset)[0]
        offset += 1
        if iv_len != 4:
            print(
                f"Only initial vector lengths of 4 bytes are supported for encrypted chunks, "
                f"given {iv_len}",
                file=sys.stderr)
            return False

        iv = data[offset:offset + iv_len]
        offset += iv_len
        normalized_iv = b''

        for i, b in enumerate(iv):
            val = (self.id >> (i * 8)) & 0xff
            normalized_iv += (b ^ val).to_bytes(1, byteorder='little')

        type_ = struct.unpack_from('<c', data, offset)[0]
        offset += 1

        if type_ != b'S':
            print(
                f"Only salsa20 encryption supported, given \"{type_.decode('ascii')}\"",
                file=sys.stderr)
            return False

        """
        sys.stderr.write('Encrypted chunk %d, type=%s, key_name_len=%d, key_name=%s, iv_len=%d iv=%s (%s), sz=%d, c_len=%d, o_len=%d offset=%d\n' % (
            self.id, type_.decode('ascii'), key_name_len, binascii.hexlify(key_name).decode('ascii'), iv_len,
            binascii.hexlify(iv).decode('ascii'), binascii.hexlify(normalized_iv).decode('ascii'),
            len(data) - offset, self.chunk_length, self.output_length, offset
        ))
        """

        key = keyfile.find(key_name)
        # No encryption key in the database, just write zeros
        if not key:
            self.output_data = b'\x00' * self.output_length
            return True

        state = salsa20.initialize(key, normalized_iv)
        tmp_data = salsa20.decrypt(state, data[offset:])

        self.is_decrypted = True

        # Run chunk processing once more, since the decrypted data is now a valid
        # chunk to perform (a non-decryption) BLTE operation on
        return self.__process(tmp_data, True)

    def __process(self, data, recursive=False):
        type_ = data[0]
        if type_ == _NULL_CHUNK:
            self.output_data = b''
        elif type_ == _UNCOMPRESSED_CHUNK:
            self.output_data = data[1:]
        elif type_ == _COMPRESSED_CHUNK:
            return self.__process_compressed(data)
        elif type_ == _ENCRYPTED_CHUNK:
            if recursive:
                print("ERROR: Encrypted chunk within encrypted chunk not supported",
                      file=sys.stderr)
                self.output_data = b'\x00' * self.output_length
                return False

            return self.__process_encrypted(data)
        else:
            print(f"Unknown chunk {self.id}, type={data[0]:#x}, sz={len(data)}, "
                  f"out_len={self.output_length}",
                  file=sys.stderr)
            self.output_data = b'\x00' * self.output_length

        return True

    def __verify(self, data):
        md5s = hashlib.md5(data).digest()
        if md5s != self.sum:
            print(
                f"Chunk{self.id} of type {data[0]:#x} fails verification, expects {self.sum.hex()} "
                f"got {md5s.hex()}",
                file=sys.stderr)
            return False

        return True


class BLTEFile:
    def __init__(self, extractor):
        self.data = extractor
        self.offset = 0
        self.chunks = []
        self.output_data = b''
        self.extract_status = True
        self.data_md5 = None

    def add_chunk(self, length, c_length, md5s):
        self.chunks.append(BLTEChunk(len(self.chunks), length, c_length, md5s))

    def fully_decrypted(self):
        return False not in [chunk.decrypted() for chunk in self.chunks]

    def __read(self, bytes_):
        if isinstance(self.data, BLTEExtract):
            return self.data.fd.read(bytes_)

        nibble = self.data[self.offset:self.offset + bytes_]
        self.offset += bytes_
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

        return self.offset

    def __extract_direct(self):
        type_ = self.__read(_MARKER_LEN)
        if type_ != _COMPRESSED_CHUNK:
            print(f"Direct extraction only supports compressed data, was given {type_:#x}",
                  file=sys.stderr)
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
        if self.__read(4) != _BLTE_MAGIC:
            print("Invalid BLTE magic in file", file=sys.stderr)
            return False

        chunk_data_offset = struct.unpack('>I', self.__read(_CHUNK_DATA_OFFSET_LEN))[0]
        if chunk_data_offset == 0:
            if not self.__extract_direct():
                return False
        else:
            unk_1, cc_b1, cc_b2, cc_b3 = struct.unpack('BBBB', self.__read(_CHUNK_HEADER_LEN))
            if unk_1 != 0x0F:
                print(
                    f"Unknown magic byte {unk_1} {self.__tell():#x} in BLTE "
                    f"@{self.__tell() - _CHUNK_HEADER_LEN:#.8x}",
                    file=sys.stderr)
                return False

            n_chunks = (cc_b1 << 16) | (cc_b2 << 8) | cc_b3
            if n_chunks == 0:
                return False

            # Chunk information
            for chunk_id in range(0, n_chunks):
                c_len, out_len = struct.unpack(
                    '>II', self.__read(_CHUNK_HEADER_2_LEN))
                chunk_sum = self.__read(_CHUNK_SUM_LEN)

                # print(f"Chunk#{chunk_id}@{self.__tell() - 24}: c_len={c_len} out_len={out_len} "
                #       f"sum={chunk_sum.hex()}")
                self.add_chunk(c_len, out_len, chunk_sum)

            # Read chunk data
            sum_in_file = 0
            for chunk_id in range(0, n_chunks):
                chunk = self.chunks[chunk_id]

                # print(f"Chunk#{chunk_id}@{self.__tell()}: Data extract len={chunk.chunk_length} "
                #       f"total={sum_in_file}")
                data = self.__read(chunk.chunk_length)
                if not chunk.extract(data):
                    self.extract_status = False
                    return False

                self.output_data += chunk.output_data
                sum_in_file += len(chunk.output_data)

        return True

    def verify(self, md5):
        # If we cannot decrypt the BLTE file, we cannot do MD5 validation
        if md5 is None or not self.fully_decrypted():
            return True

        self.data_md5 = hashlib.md5(self.output_data).digest()

        return md5 == self.data_md5


class BLTEExtract:
    def __init__(self, options):
        self.options = options

    def open(self, data_file):
        if not os.access(data_file, os.R_OK):
            self.options.parser.error(f"File {data_file} not readable.")

        self.fsize = os.stat(data_file).st_size
        if self.fsize == 0:
            self.options.parser.error(f"File {data_file} is empty.")

        with open(data_file, 'rb') as handle:
            self.fdesc = handle
            self.fd = mmap.mmap(handle.fileno(), 0, access=mmap.ACCESS_READ)

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

    def extract_buffer_to_file(self, data, fname, md5=None):
        dirname = os.path.dirname(os.path.normpath(fname))
        if not os.path.exists(dirname):
            os.makedirs(dirname)

        data = self.extract_buffer(data, md5)
        if not data:
            print(f"Unable to extract {os.path.basename(fname)} ...", file=sys.stderr)
            return False

        with open(fname, 'wb') as f:
            f.write(data)

        return True

    def extract_buffer(self, data, md5=None):
        file = BLTEFile(data)
        if not file.extract():
            return None

        if not file.verify(md5):
            data_md5 = hashlib.md5(data).digest()
            print(
                f"Unable to extract buffer, invalid md5ums, got={data_md5.hex()}, "
                f"expected={md5.hex()}",
                file=sys.stderr)
            return None

        return file.output_data

    def extract_blte_file(self, file_name):
        if not self.open(file_name):
            return False

        if not os.access(self.options.output, os.W_OK):
            self.options.parser.error(f"Output file {self.options.output} is not writeable")

        file = self.__extract_file()
        if not file:
            return False

        self.close()

        return True

    def extract_data(self, file_key, file_md5sum, data_file_number,
                     data_file_offset, blte_file_size):
        path = os.path.join(self.options.data_dir, 'Data', 'data', f'data.{data_file_number:03d}')

        if not self.open(path):
            return None

        self.fd.seek(data_file_offset, os.SEEK_SET)

        key = self.fd.read(16)
        blte_len = struct.unpack('<I', self.fd.read(4))[0]

        if blte_len != blte_file_size:
            self.options.parser.error(
                f'Invalid file length, expected {blte_file_size} got {blte_len}'
            )

        # Key is apparently in reverse byte order, and only 9 bytes of it are relevant (as in the
        # index structures)
        for idx in range(0, 9):
            if file_key[idx] != key[len(key) - 1 - idx]:
                self.options.parser.error(
                    f"Invalid file key for {codecs.encode(file_key, 'hex').decode('utf-8')}, "
                    f"got {codecs.encode(key, 'hex').decode('utf-8')}")

        # Skip 10 bytes of unknown data
        self.fd.seek(10, os.SEEK_CUR)

        file = self.__extract_file()
        if not file:
            return None

        if not file.verify(file_md5sum):
            self.options.parser.error(
                f"Invalid md5sum for extracted file, expected {file_md5sum.hex()}"
                f" got {file.data_md5.hex()}")
            return None

        self.close()

        return file.output_data

    def extract_file(self, file_key, file_md5sum, file_output,
                     data_file_number, data_file_offset, blte_file_size):
        output_path = ''
        if file_output:
            output_path = os.path.join(self.options.output, file_output)
        else:
            if file_key:
                output_path = os.path.join(self.options.output, file_key.hex())
            elif file_md5sum:
                output_path = os.path.join(self.options.output,
                                           file_md5sum.hex())

        output_dir = os.path.dirname(os.path.abspath(output_path))
        try:
            if not os.path.exists(output_dir):
                os.makedirs(output_dir)
        except os.error as e:
            self.options.parser.error('Output "%s" is not writable: %s' %
                                      (output_path, e.strerror))

        data = self.extract_data(file_key, file_md5sum, data_file_number,
                                 data_file_offset, blte_file_size)
        if not data:
            return False

        try:
            with open(output_path, 'wb') as output_file:
                output_file.write(data)
        except IOError as e:
            self.options.parser.error('Output "%s" is not writable: %s' %
                                      (output_path, e.strerror))

        return True


class CASCObject:
    def __init__(self, options):
        self.options = options

    def get_url(self, url, headers=None):
        attempt = 0
        maxAttempts = 5
        while attempt < maxAttempts:
            r = None
            try:
                sys.stdout.write('Fetching %s ...\n' % url)
                r = _S.get(url, headers=headers)

                # Handle a 404 like a communication error, will retry and change the CDN
                if r.status_code == 404:
                    raise Exception('404')

                if r.status_code not in [200, 206]:
                    self.options.parser.error(
                        'HTTP request for %s returns %u' %
                        (url, r.status_code))
                return r
            except Exception as e:
                print(
                    f"Unable to fetch {url} (attempt {attempt}): "
                    f"{r.reason if r else 'unknown error'}: {e}",
                    file=sys.stderr)
                if attempt + 1 < maxAttempts:
                    print(f"Retrying {url} ...")
                    # try a random, different CDN host
                    if len(self.cdn_host):
                        cur_host = ''
                        for host in self.cdn_host:
                            if url.find(host):
                                cur_host = host
                        if cur_host:
                            other_hosts = [host for host in self.cdn_host if host != cur_host]
                            url = url.replace(cur_host, random.choice(other_hosts))
                    time.sleep(2**attempt)
                    attempt += 1
        self.options.parser.error(
            'Unable to fetch CDN URL file %s (too many retries), aborting ...' %
            url)

    def cache_dir(self, path=None):
        dir = self.options.cache
        if path:
            dir = os.path.join(self.options.cache, path)

        if not os.path.exists(dir):
            try:
                os.makedirs(dir)
            except os.error as e:
                self.options.parser('Unable to make %s: %s' %
                                    (dir, e.strerror))

        return dir

    def write_cache(self, file, handle):
        if handle is None or not handle.seekable() or handle.closed or not handle.readable():
            return

        try:
            handle.seek(0, os.SEEK_SET)
            with open(os.path.abspath(file), 'wb') as f:
                f.write(handle.read())
        except (OSError, ValueError) as err:
            self.options.parser(
                f'Unable to commit data to cache {file}, {err}')

        handle.seek(0, os.SEEK_SET)

    def cached_open(self, file, url, headers=None):
        handle = None

        if not os.path.exists(file):
            handle = self.get_url(url, headers)
            if handle.status_code < 200 or handle.status_code > 299:
                return None, False

            return io.BytesIO(handle.content), False
        else:
            return open(file, 'rb'), True


class BuildCfg:
    def __init__(self, handle):
        self.handle = handle

        for line in self.handle:
            mobj = re.match('^([^ ]+)[ ]*=[^A-z0-9]*(.+)',
                            line.decode('utf-8'))
            if not mobj:
                continue

            data = mobj.group(2)
            if ' ' in data:
                data = data.split(' ')

            key = mobj.group(1).replace('-', '_')

            setattr(self, key, data)


class CDNIndex(CASCObject):
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
        return f'http://us.patch.battle.net:1119/{build_cfg.product_arg_str(self.options)}'

    def open_cdns(self):
        cdns_url = '%s/cdns' % self.patch_base_url()
        handle = self.get_url(cdns_url)

        data = handle.text.strip().split('\n')
        for line in data:
            split = line.split('|')
            if split[0] != self.options.region:
                continue

            self.cdn_path = split[1]
            # cdns = [urllib.parse.urlparse(x).netloc for x in split[3].split(' ')]
            # self.cdn_host = cdns[-1]
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
        return '%s/%s/%s/%s/%s' % (self.cdn_base_url(), type, file[:2],
                                   file[2:4], file)

    def open_version(self):
        version_url = '%s/%s' % (self.patch_base_url(),
                                 'bgdl' if self.bgdl else 'versions')
        handle = self.get_url(version_url)

        for line in handle.iter_lines():
            split = line.decode('utf-8').strip().split('|')
            if split[0] != 'us':
                continue

            if len(split) != 7:
                sys.stderr.write(
                    'Version format mismatch, expected 7 fields, got %d\n' %
                    len(split))
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
                sys.stderr.write('Unable to parse version from "%s" data\n' %
                                 self.version)
                sys.exit(1)

            # Also take build configuration information from the versions file
            # nowadays, as the "CDN" file builds option may have things like
            # background downloader builds in it
            self.build_cfg_hash = [
                split[1],
            ]

        if not self.cdn_hash:
            sys.stderr.write('Invalid version file\n')
            sys.exit(1)

        print('Current build version: %s [%d]' %
              (self.version, self.build_number))

    def open_cdn_build_cfg(self):
        path = os.path.join(self.cache_dir('config'), self.cdn_hash)
        url = self.cdn_url('config', self.cdn_hash)

        handle, cached = self.cached_open(path, url)
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

        if not cached:
            self.write_cache(path, handle)

    def open_build_cfg(self):
        for cfg in self.build_cfg_hash:
            path = os.path.join(self.cache_dir('config'), cfg)
            url = self.cdn_url('config', cfg)
            handle, cached = self.cached_open(path, url)
            if not handle:
                self.options.parser.error(
                    'Unable to fetch build configuration')

            self.builds.append(BuildCfg(handle))

            if not cached:
                self.write_cache(path, handle)

    def open_archives(self):
        sys.stdout.write('Parsing CDN index files ... \n')

        index_cache = self.cache_dir('index')
        for idx in range(0, len(self.archives)):
            index_file_name = '%s.index' % self.archives[idx]
            index_file_path = os.path.join(index_cache, index_file_name)
            index_file_url = self.cdn_url('data', index_file_name)

            handle, cached = self.cached_open(index_file_path, index_file_url)
            if not self.parse_archive(handle, idx):
                self.options.parser.error(
                    'Unable to parse index file %s, aborting ...' %
                    index_file_name)

            if not cached:
                self.write_cache(index_file_path, handle)

        sys.stdout.write('%u entries\n' % len(self.cdn_index.keys()))

    def parse_archive(self, handle, idx):
        buf = handle.read()
        data_size = len(buf) - _ARCHIVE_IDX_FOOTER.size

        offset_footer = data_size

        footer = _ARCHIVE_IDX_FOOTER.unpack_from(buf, offset_footer)

        toc_csum = footer[0]
        version = footer[1]
        block_size = footer[4] << 10
        key_size = footer[-4]
        csum_size = footer[-3]
        n_elements = footer[-2]
        footer_csum = footer[-1]

        if version != 1:
            print(f"Unsupported archive index version {version}",
                  file=sys.stderr)
            return False

        # Validate footer
        buf_footer = buf[offset_footer + csum_size:offset_footer +
                         _ARCHIVE_IDX_FOOTER.size - csum_size]
        buf_footer += b'\x00' * csum_size

        footer_digest = hashlib.md5(buf_footer).digest()
        if footer_digest[:csum_size] != footer_csum:
            print(
                f"Footer fails checksum check,"
                f" calculated: {footer_digest[:csum_size].hex()}, expected: {footer_csum.hex()}",
                file=sys.stderr)
            return False

        if n_elements == 0:
            return True

        n_entries_per_block = block_size // _ARCHIVE_IDX_ENTRY.size
        n_blocks = int(math.ceil(n_elements / n_entries_per_block))

        offset_toc_base = n_blocks * block_size
        offset_toc_hash = offset_toc_base + n_blocks * key_size

        # Validate toc data
        toc_digest = hashlib.md5(buf[offset_toc_base:offset_toc_hash +
                                     n_blocks * csum_size]).digest()
        if toc_digest[:csum_size] != toc_csum:
            print(
                f"Toc fails checksum check,"
                f" calculated: {toc_digest[:csum_size].hex()}, expected: {toc_csum.hex()}",
                file=sys.stderr)
            return False

        toc_block_lastkey = []
        toc_block_hash = []
        for record_idx in range(0, n_blocks):
            record_offset = offset_toc_base + record_idx * key_size
            toc_block_lastkey.append(buf[record_offset:record_offset +
                                         key_size])

        for record_idx in range(0, n_blocks):
            record_offset = offset_toc_hash + record_idx * csum_size
            toc_block_hash.append(buf[record_offset:record_offset + csum_size])

        # Validate block data
        for block_idx in range(0, n_blocks):
            offset_block = block_idx * block_size
            block_digest = hashlib.md5(buf[offset_block:offset_block +
                                           block_size]).digest()
            if block_digest[:csum_size] != toc_block_hash[block_idx]:
                print(
                    f"Block #{block_idx} fails checksum check,"
                    f" calculated: {block_digest[:csum_size].hex()},"
                    f" expected: {toc_block_hash[block_idx].hex()}",
                    file=sys.stderr)
                return False

        block_idx = 0
        offset_data = 0
        for record_idx in range(0, n_elements):
            key, size, offset = _ARCHIVE_IDX_ENTRY.unpack_from(
                buf, offset_data)
            offset_data += _ARCHIVE_IDX_ENTRY.size

            if key in self.cdn_index:
                print(
                    f"Key {key.hex()} (idx={idx}, size={size}, offset={offset}) exists in "
                    f"index @ ({self.archives[self.cdn_index[key].index]}, "
                    f"{self.cdn_index[key].size}, {self.cdn_index[key].offset}), overwriting ...",
                    file=sys.stderr)

            self.cdn_index[key] = CDNIndexRecord(idx, size, offset)

            if key == toc_block_lastkey[block_idx]:
                offset_data += block_size - offset_data % block_size
                block_idx += 1

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
        cached = False
        key_file_path = os.path.join(self.cache_dir('data'), key.hex())
        if key_info:
            key_file_url = self.cdn_url('data', self.archives[key_info.index])

            handle, cached = self.cached_open(
                key_file_path, key_file_url, {
                    'Range':
                    'bytes=%d-%d' %
                    (key_info.offset, key_info.offset + key_info.size - 1)
                })
        else:
            handle, cached = self.cached_open(
                key_file_path,
                self.cdn_url('data', key.hex()))

        if not handle:
            return None

        if not cached:
            self.write_cache(key_file_path, handle)

        return handle.read()


class RibbitIndex(CDNIndex):
    def get_url(self, content, headers=None):
        if headers or 'http://' in content:
            return super().get_url(content, headers)
        else:
            s = socket.create_connection(('us.version.battle.net', 1119), 10)
            s.send(bytes(content + '\r\n', 'ascii'))
            return s.makefile('b')

    def patch_base_url(self):
        return f'v1/products/{build_cfg.product_arg_str(self.options)}'


class CASCDataIndexFile(object):
    def __init__(self, options, index, version, file):
        self.index = index
        self.version = version
        self.file = file
        self.options = options

    def _byte_size(self, sz):
        if sz == 8:
            return 'Q'
        elif sz == 4:
            return 'I'
        elif sz == 2:
            return 'H'
        elif sz == 1:
            return 'B'
        else:
            return f'{sz}s'

    def open(self):
        if not os.access(self.file, os.R_OK):
            self.options.parser.error(f"Unable to read index file {self.file}")

        with open(self.file, 'rb') as f:
            data = f.read()

        ptr = 0
        header = _LOCAL_IDX_HEADER.unpack_from(data, ptr)

        hdr_size = header[0]
        hdr_csum = header[1]
        entry_sz_bytes = header[5]
        entry_ofs_bytes = header[6]
        entry_key_bytes = header[7]
        entries_size = header[-2]
        # entries_csum = header[-1]

        if entry_sz_bytes != 4 or entry_ofs_bytes != 5 or entry_key_bytes != 9:
            print(
                f"Unknown header format for {self.file}, entry_sz={entry_sz_bytes} (expected 4)"
                f", entry_ofs={entry_ofs_bytes} (expected 5), "
                f"entry_key={entry_key_bytes} (expected 16)",
                file=sys.stderr)
            return False

        csum = jenkins.hashlittle2(data[8:8 + hdr_size])
        if csum[0] != hdr_csum:
            print(
                f"Unable to validate header for {self.file}, "
                f"expected checksum {hdr_csum:#0x}, computed checksum {csum[0]:#0x}",
                file=sys.stderr)
            return False

        ptr += _LOCAL_IDX_HEADER.size
        """
        csum = jenkins.hashlittle2(data[ptr:ptr + entries_size])
        if csum[0] != entries_csum:
        print(f"Unable to validate data for {self.file}, "
            f"expected checksum {entries_csum:#0x}, computed checksum {csum[0]:#0x}",
            file=sys.stderr)
        return False
        """

        # Create parsing struct dynamically
        parser = struct.Struct(
            f"<{entry_key_bytes}s{entry_ofs_bytes}s{self._byte_size(entry_sz_bytes)}"
        )

        while ptr < _LOCAL_IDX_HEADER.size + entries_size:
            key, ofs_bytes, size = parser.unpack(data[ptr:ptr + parser.size])

            offset = int.from_bytes(ofs_bytes, byteorder='big')
            archive_offset = offset & 0x3FFFFFFF
            archive_number = offset >> 30

            self.index.AddIndex(key, archive_number, archive_offset, size, self.file)

            ptr += parser.size

        return True


class CASCDataIndex:
    def __init__(self, options):
        self.options = options
        self.idx_files = {}
        self.idx_data = {}

    def AddIndex(self, key, data_file, data_offset, file_size, indexname):
        if key not in self.idx_data:
            self.idx_data[key] = (data_file, data_offset, file_size)
        else:
            print(f"Duplicate key {key.hex()} in index {indexname} old={self.idx_data[key]}, "
                  f"new={(data_file, data_offset, file_size)}",
                  file=sys.stderr)

    def GetIndexData(self, key):
        return self.idx_data.get(key[:9], (-1, 0, 0))

    def open(self):
        data_dir = os.path.join(self.options.data_dir, 'Data', 'data')
        if not os.access(data_dir, os.R_OK):
            self.options.parser.error(
                'Unable to read World of Warcraft data directory %s' %
                data_dir)
            return False

        idx_files_glob = os.path.join(data_dir, '*.idx')
        idx_files = glob.glob(idx_files_glob)
        if len(idx_files) == 0:
            self.options.parser.error(
                'No index files found in World of Warcraft data directory %s' %
                data_dir)
            return False

        idx_file_re = re.compile('([0-9a-fA-F]{2})([0-9a-fA-F]{8})')

        for idx_file in idx_files:
            mobj = idx_file_re.match(os.path.basename(idx_file))
            if not mobj:
                self.options.parser.error('Unable to match filename %s' %
                                          idx_file)
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
        self.md5_map = {}

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
        elif self.options.product:
            base_cached_name += f'.{self.options.product}'

        return os.path.join(self.cache_dir(), base_cached_name)

    def __bootstrap(self):
        if not os.access(self.cache_dir(), os.W_OK):
            self.options.parser.error(
                f"Error bootstrapping CASCEncodingFile, \"{self.cache_dir()}\" is not writable")

        encoding_file_path = os.path.join(self.cache_dir('data'), self.build.encoding_file())
        handle, cached = self.cached_open(encoding_file_path, self.build.encoding_blte_url())
        # handle = self.get_url(self.build.encoding_blte_url())
        if not handle:
            self.options.parser.error('Unable to fetch encoding file')

        blte = BLTEFile(handle.read())
        if not blte.extract():
            self.options.parser.error('Unable to uncompress BLTE data for encoding file')

        md5s = hashlib.md5(blte.output_data).hexdigest()
        if md5s != self.build.encoding_file():
            self.options.parser.error(
                f"Invalid md5sum in encoding file, expected {self.build.encoding_file()} "
                f"got {md5s}")

        with open(self.encoding_path(), 'wb') as f:
            f.write(blte.output_data)

        if not cached:
            self.write_cache(encoding_file_path, handle)

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

        print(f"Parsing encoding file {self.build.encoding_file()} ...")

        offset = 0
        magic = data[offset:offset + 2]
        offset += 2
        if magic != b'EN':
            self.options.parser.error(
                f"Unknown magic \"{self.options.locale}\" in encoding file {self.encoding_path()}")
            return False

        unk_b1, unk_b2, unk_b3, unk_s1, unk_s2, \
            hash_table_size, unk_hash_count, unk_b4, hash_table_offset = \
            struct.unpack('>BBBHHIIBI', data[offset:offset + 20])

        # print(unk_b1, unk_b2, unk_b3, unk_s1, unk_s2, hash_table_size,
        #       unk_hash_count, unk_b4, hash_table_offset)
        offset += 20 + hash_table_offset

        for hash_entry in range(0, hash_table_size):
            md5_file = data[offset:offset + 16]
            offset += 16
            # md5_sum = data[offset:offset + 16];
            # Skip the second MD5sum, we don't use it for anything
            offset += 16
            self.first_entries.append(md5_file)

        for hash_block in range(0, hash_table_size):
            before = offset
            entry_id = 0
            # print('Block %u begins at %u' % (hash_block, f.tell()))
            n_keys = struct.unpack('H', data[offset:offset + 2])[0]
            offset += 2
            while n_keys != 0:
                keys = []
                file_size = struct.unpack('>I', data[offset:offset + 4])[0]
                offset += 4
                file_md5 = data[offset:offset + 16]
                offset += 16
                for key_idx in range(0, n_keys):
                    keys.append(data[offset:offset + 16])
                    offset += 16

                if entry_id == 0 and file_md5 != self.first_entries[hash_block]:
                    self.options.parser.error(
                        'Invalid first md5 in block %d@%u, expected %s got %s'
                        % (hash_block, offset,
                           codecs.encode(self.first_entries[hash_block],
                                         'hex').decode('utf-8'),
                           codecs.encode(file_md5, 'hex').decode('utf-8')))
                    return False

                # print('%5u %8u %8u %2u %s %s' % (entry_id, f.tell(), file_size,
                #      n_keys, file_md5.encode('hex'), file_key.encode('hex')))
                if file_size > 1000000000:
                    self.options.parser.error(
                        'Invalid (too large) file size %u in block %u, entry id %u, pos %u'
                        % (file_size, hash_block, entry_id, offset))
                    return False

                if file_md5 in self.md5_map:
                    self.options.parser.error(
                        'Duplicate md5 entry %s in block %d@%u' %
                        (codecs.encode(file_md5, 'hex').decode('utf-8'),
                         hash_block, offset))
                    return False

                self.md5_map[file_md5] = (file_size, keys)

                entry_id += 1
                n_keys = struct.unpack('H', data[offset:offset + 2])[0]
                offset += 2

            # Blocks are padded to 4096 bytes it seems, sanity check that the
            # padding is all zeros, though
            for pad_idx in range(0, 4096 - (offset - before)):
                byte = data[offset:offset + 1]
                offset += 1
                if byte != b'\x00':
                    self.options.parser.error(
                        f"Invalid padding byte {pad_idx} at the end of block {hash_block}, "
                        f"pos {offset}, expected 0, got {byte:#x}")
                    return False

        sys.stdout.write('%u entries\n' % len(self.md5_map.keys()))

        # Rest of the encoding file is unnecessary for now
        """
        for i in range(0, unk_hash_count):
          tmp = offset
          hash = data[offset:offset + 16]; offset += 16
          hash2 = data[offset:offset + 16]; offset += 16
          unk_word1, unk_word2 = struct.unpack('>II', data[offset:offset + 8]); offset += 8
          unk_b1 = data[offset]; offset += 1
          print(tmp, hash.encode('hex'), hash2.encode('hex'))

        n = 0
        start = offset
        while offset < fsize:
            tmp = offset
            hash = data[offset:offset + 16]; offset += 16
            opt1 = struct.unpack('>IBI', data[offset:offset + 9])
            self.key_map[hash] = opt1
            offset += 9
            if opt1[0] == 0xFFFFFFFF:
                break
            print(n, tmp, 4096 - (offset - start), offset - start, hash.encode('hex'), opt1)
            n += 1
            if 4096 - (offset - start) < 25:
                offset += 4096 - (offset - start)
                start = offset

        print(offset)
        """

        return True


class CASCRootFile(CASCObject):
    # en_US locale
    _locale = {
        'en_US': 0x2,
        'ko_KR': 0x4,
        'fr_FR': 0x10,
        'de_DE': 0x20,
        'zh_CN': 0x40,
        'es_ES': 0x80,
        'zh_TW': 0x100,
        'en_GB': 0x200,
        'en_CN': 0x400,
        'en_TW': 0x800,
        'es_MX': 0x1000,
        'ru_RU': 0x2000,
        'pt_BR': 0x4000,
        'it_IT': 0x8000,
        'pt_PT': 0x10000
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
            self.options.parser.error('Invalid locale, valid values are %s' %
                                      (', '.join(CASCRootFile._locale.keys())))

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
        elif self.options.product:
            base_cached_name += f'.{self.options.product}'

        return os.path.join(self.cache_dir(), base_cached_name)

    def __bootstrap(self):
        if not os.access(self.cache_dir(), os.W_OK):
            self.options.parser.error(
                'Error bootstrapping CASCRootFile, "%s" is not writable' %
                self.cache_dir())

        keys = self.encoding.GetFileKeys(codecs.decode(self.build.root_file(), 'hex'))
        if len(keys) == 0:
            self.options.parser.error(
                'Could not find root file with mdsum %s from data' %
                self.build.root_file())

        blte = BLTEExtract(self.options)
        # Extract from local files
        if not self.options.online:
            file_locations = []
            for key in keys:
                idx = self.index.GetIndexData(key)
                if idx[0] == -1:
                    continue

                file_locations.append((key, codecs.decode(self.build.root_file(), 'hex')) + idx)

            if len(file_locations) == 0:
                self.options.parser.error(f"Could not find root file "
                                          f"(keys={', '.join([k.hex() for k in keys])}) "
                                          f"location in index.")

            if len(file_locations) > 1:
                self.options.parser.error(
                    'Multiple (%d) file locations for root file in data' %
                    len(file_locations))

            data = blte.extract_data(*file_locations[0])
        # Fetch from online
        else:
            if len(keys) > 1:
                print('Duplicate root key found for %s, using first one ...' %
                      self.build.root_file())

            root_file_path = os.path.join(self.cache_dir('data'),
                                          self.build.root_file())
            handle, cached = self.cached_open(root_file_path,
                                              self.build.cdn_url('data', keys[0].hex()))

            if not handle:
                self.options.parser.error('Unable to fetch root file')

            blte = BLTEFile(handle.read())
            if not blte.extract():
                self.options.parser.error(
                    'Unable to uncompress BLTE data for root file')

            md5s = hashlib.md5(blte.output_data).hexdigest()
            if md5s != self.build.root_file():
                self.options.parser.error(
                    'Invalid md5sum in root file, expected %s got %s' %
                    (self.build.root_file(), md5s))

            if not cached:
                self.write_cache(root_file_path, handle)

            data = blte.output_data

        with open(self.root_path(), 'wb') as f:
            f.write(data)

        return data

    def GetFileDataIdMD5(self, file_data_id):
        return self.hash_map.get(int(file_data_id), [])

    def GetFilePathMD5(self, hash):
        return self.path_hash_map.get(hash, [])

    def GetLocale(self):
        if self.options.locale not in CASCRootFile._locale:
            return 0

        flags = CASCRootFile._locale[self.options.locale]
        if self.options.locale in ['en_US', 'en_GB']:
            flags = CASCRootFile._locale['en_US'] | CASCRootFile._locale[
                'en_GB']

        return flags

    def __parse_default(self, data):
        offset = 0
        n_md5s = 0

        hdr_size = 0
        # version = 0
        # total_file_count = 0
        # named_file_count = 0
        unk_h5 = 0

        magic, unk_h1, unk_h2 = _ROOT_HEADER.unpack_from(data, offset)

        if magic != _ROOT_MAGIC:
            print('Invalid magic in file, expected "{:s}", got "{:s}"'.format(
                _ROOT_MAGIC.decode('ascii'), magic.decode('ascii')))
            return False
        offset += _ROOT_HEADER.size

        if unk_h1 <= _ROOT_HEADER.size + _ROOT_HEADER_2.size:
            hdr_size = unk_h1
            # version = unk_h2
            total_file_count, named_file_count, unk_h5 = _ROOT_HEADER_2.unpack_from(
                data, offset)
            offset = hdr_size
        else:
            hdr_size = _ROOT_HEADER.size
            # total_file_count = unk_h1
            # named_file_count = unk_h2

        # print(magic, hdr_size, version, total_file_count, named_file_count, unk_h5)

        while offset < len(data):
            n_entries, flags, locale = struct.unpack_from('<iII', data, offset)
            # print('offset', offset, 'n-entries', n_entries, 'content_flags',
            #      '{:#8x}'.format(flags), 'locale', '{:#8x}'.format(locale))

            offset += 12
            if n_entries == 0:
                continue

            findex = struct.unpack_from('<{}I'.format(n_entries), data, offset)
            offset += 4 * n_entries

            csum_file_id = 0
            file_ids = []
            for entry_idx in range(0, n_entries):
                md5s = data[offset:offset + 16]
                offset += 16

                file_data_id = 0
                if entry_idx == 0:
                    file_data_id = findex[entry_idx]
                else:
                    file_data_id = csum_file_id + 1 + findex[entry_idx]
                csum_file_id = file_data_id

                if locale != CASCRootFile.LOCALE_ALL and not (
                        locale & self.GetLocale()):
                    file_ids.append(None)
                    continue

                if file_data_id not in self.hash_map:
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
                    if file_ids[entry_idx] is None:
                        continue

                    if hashes[entry_idx] not in self.path_hash_map:
                        self.path_hash_map[hashes[entry_idx]] = []
                        self.path_hash_map[
                            hashes[entry_idx]] += self.hash_map[file_data_id]

                # file_path_hash = _PATH_HASH.unpack_from(data, offset)[0]
                offset += _PATH_HASH.size

        sys.stdout.write('%u entries\n' % n_md5s)

        return True

    def __parse_classic(self, data):
        offset = 0
        n_md5s = 0

        _PATH_HASH = struct.Struct('<Q')

        while offset < len(data):
            n_entries, flags, locale = struct.unpack_from('<iII', data, offset)
            # print('offset', offset, 'n-entries', n_entries, 'content_flags',
            #      '{:#8x}'.format(flags), 'locale', '{:#8x}'.format(locale))
            offset += 12
            if n_entries == 0:
                continue

            findex = struct.unpack_from('<{}I'.format(n_entries), data, offset)
            offset += 4 * n_entries

            csum_file_id = 0
            for entry_idx in range(0, n_entries):
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

                if locale != CASCRootFile.LOCALE_ALL and not (
                        locale & self.GetLocale()):
                    continue

                if file_data_id not in self.hash_map:
                    self.hash_map[file_data_id] = []

                self.hash_map[file_data_id].append(md5s)

                if hash_ not in self.path_hash_map:
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

        sys.stdout.write('Parsing root file %s ... \n' %
                         self.build.root_file())

        if len(data) < 4:
            sys.stdout.write('Root file too small (%d bytes)\n' % len(data))
            return False

        if data[:4] == b'TSFM':
            return self.__parse_default(data)
        else:
            return self.__parse_classic(data)
