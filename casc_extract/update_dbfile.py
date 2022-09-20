import mmap, sys

try:
  from pefile import PE
except Exception as error:
  print(f'ERROR: {error}, update_dbfile.py requires the Python pefile (https://pypi.org/project/pefile/)', file=sys.stderr)
  exit(1)

class Field:
  def __init__(self, name, pattern=b'****'):
    self.pattern = pattern
    self.name = name

  def unpack_value(self, bytes_, struct_parser):
    return int.from_bytes(bytes_, 'little')

  def is_valid(self, struct, struct_parser):
    return True # By default, any value that gets unpacked is allowed.

class Padding(Field):
  def __init__(self, name, length=4, byte=b'\00'):
    super().__init__(name, byte * length)

class Pointer(Field):
  def __init__(self, name):
    super().__init__(name, b'****\01\00\00\00')

  def is_valid(self, struct, struct_parser):
    return struct_parser.get_address(struct[self.name]) < struct_parser.handle.size()

class StringPointer(Pointer):
  def __init__(self, name, encoding='ascii'):
    super().__init__(name)
    self.encoding = encoding

  def unpack_value(self, bytes_, struct_parser):
    virtual_address = int.from_bytes(bytes_, 'little')
    return struct_parser.read_string(virtual_address, self.encoding) # This will throw an exception if the address is not valid.

  def is_valid(self, struct, struct_parser):
    return struct[self.name] is not None

class FileDataID(Field):
  def is_valid(self, struct, struct_parser):
    value = struct[self.name]
    return value > 100000 and value < 20000000

class Hash(Field):
  def __init__(self, name, length=4, hashed_field=None, hash_function=None):
    super().__init__(name)
    self.pattern = b'*' * length
    self.hashed_field = hashed_field
    self.hash_function = hash_function

  def is_valid(self, struct, struct_parser):
    if self.hashed_field is not None and self.hash_function is not None:
      return struct[self.name] == self.hash_function(struct[self.hashed_field])
    return super().is_valid(struct_parser, struct)

# Instances of StructParser must define the 'fields' attribute, which should be an ordered iterable containing the Fields of the struct.
class StructParser:
  struct_location = 'rdata'

  def __init__(self, binary_path):
    self.buffer_size = 1024
    with open(binary_path, 'rb') as f:
      self.handle = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
      pe = PE(data=self.handle, fast_load=True)
      self.image_base = pe.OPTIONAL_HEADER.ImageBase
      for section in pe.sections:
        if b'.data' in section.Name:
          self.data_start = section.PointerToRawData
          self.data_end = self.data_start + section.SizeOfRawData
          self.data_virtual_address = section.VirtualAddress
        if b'.rdata' in section.Name:
          self.rdata_virtual_address = section.VirtualAddress
          self.rdata_start = section.PointerToRawData
          self.rdata_end = self.rdata_start + section.SizeOfRawData

  # only gives valid addresses for .rdata and .data pointers
  def get_address(self, virtual_address):
    rdata_address = virtual_address - self.image_base - self.rdata_virtual_address + self.rdata_start
    if rdata_address >= self.rdata_start and rdata_address < self.rdata_end:
      return rdata_address

    data_address = virtual_address - self.image_base - self.data_virtual_address + self.data_start
    if data_address >= self.data_start and data_address < self.data_end:
      return data_address

    return self.handle.size()

  def read_string(self, virtual_address, encoding):
    cur = self.handle.tell()
    pos = self.get_address(virtual_address)
    if pos >= self.handle.size():
      return None
    try:
      self.handle.seek(pos)
      offset = self.handle.find(b'\00', pos)
      string = self.handle.read(offset - pos).decode(encoding=encoding)
      self.handle.seek(cur)
    except Exception as e:
      self.handle.seek(cur)
      raise e
    return string

  def start(self):
    return getattr(self, f'{self.struct_location}_start')

  def end(self):
    return getattr(self, f'{self.struct_location}_end')

  def pattern_match(self, buffer, offset):
    fields_offset = 0
    for field in self.fields:
      for i, byte in enumerate(field.pattern):
        if byte != ord('*') and buffer[offset + fields_offset + i] != byte:
          return False
      fields_offset += len(field.pattern)
    return True

  def unpack_struct(self, buffer, offset):
    struct = {}
    fields_offset = 0
    for field in self.fields:
      struct[field.name] = field.unpack_value(buffer[offset + fields_offset:offset + fields_offset + len(field.pattern)], self)
      fields_offset += len(field.pattern)
    for field in self.fields:
      if not field.is_valid(struct, self):
        raise ValueError(f'Field {field.name}={struct[field.name]} is not valid')
    return struct

  def find_structs(self):
    temp_matches = []
    matches = []
    pattern_length = 0
    for field in self.fields:
      pattern_length += len(field.pattern)
    self.handle.seek(self.start())
    while self.handle.tell() + pattern_length < self.end():
      offset = 0
      self.handle.seek(self.handle.tell() - pattern_length)
      buffer = self.handle.read(self.buffer_size)
      while offset + pattern_length < self.buffer_size:
        if self.pattern_match(buffer, offset):
          try:
            matches.append(self.unpack_struct(buffer, offset))
          except ValueError:
            pass # ValueError indicates an invalid match
        offset += 1
    return matches

  def print_structs(self):
    matches = self.find_structs()
    for match in matches:
      print(match)

class GameTableMetaParser(StructParser):
  # table metadata is located in .data and appears to have the following structure
  struct_location = 'data'
  fields = (
    Field('index', pattern=b'*\00\00\00'), # starts at 0 and increases by 1 for each table
    Padding('pad', length=4),
    StringPointer('name'),
    FileDataID('file_data_id'),
    Field('unk_5', pattern=b'*\00\00\00'),
    Field('unk_6', pattern=b'*\00\00\00'),
    Field('unk_7', pattern=b'*\00\00\00'), # bool?
  )

# From https://wowdev.wiki/SStrHash
def SStrHash(string):
  def upper(c):
    if c == '/':
      return '\\'
    return c.upper()
  hash_table = [0x486E26EE, 0xDCAA16B3, 0xE1918EEF, 0x202DAFDB, 0x341C7DC7, 0x1C365303, 0x40EF2D37, 0x65FD5E49,
                0xD6057177, 0x904ECE93, 0x1C38024F, 0x98FD323B, 0xE3061AE7, 0xA39B0FA1, 0x9797F25F, 0xE4444563]
  seed = 0x7FED7FED
  shift = 0xEEEEEEEE
  for c in string:
    c = ord(upper(c))
    # In Python, we need to mask with 0xFFFFFFFF in two places here so that the u32 math is correct.
    seed = (hash_table[c >> 4] - hash_table[c & 0xF]) & 0xFFFFFFFF ^ (shift + seed) & 0xFFFFFFFF
    shift = c + seed + 33 * shift + 3
  return seed if seed else 1

class DB2MetaParser(StructParser):
  # db2 metadata is located in .rdata and appears to have the following structure
  fields = (
    StringPointer('name'),
    FileDataID('file_data_id'),
    Field('fields'),
    Field('size'),
    Field('unk_5'),
    Field('unk_6'),
    Field('unk_7'),
    Pointer('va_byte_offset'),
    Pointer('va_elements'),
    Pointer('va_field_format'),
    Pointer('va_flags'),
    Pointer('va_elements_file'),
    Pointer('va_field_format_file'),
    Pointer('va_flags_file'),
    Field('unk_15'),
    Hash('table_hash', hashed_field='name', hash_function=SStrHash),
    Field('unk_17'),
    Hash('layout_hash'),
    Field('unk_19'),
  )

def update_dbfile(binary_path, dbfile_path):
  old_files = {}
  new_files = {}
  try:
    with open(dbfile_path, 'r') as f:
      for line in f:
        if line.strip():
          file_data_id, file_path = line.split(', ', 1)
          old_files[int(file_data_id)] = file_path.strip()
  except FileNotFoundError:
    pass
  for struct in GameTableMetaParser(binary_path).find_structs():
    new_files[struct['file_data_id']] = f'GameTables\\{struct["name"]}.txt'
  for struct in DB2MetaParser(binary_path).find_structs():
    new_files[struct['file_data_id']] = f'DBFilesClient\\{struct["name"]}.db2'
  with open(dbfile_path, 'w') as f:
    for file_data_id in sorted(set(old_files) | set(new_files)):
      if file_data_id in new_files:
        f.write(f'{file_data_id}, {new_files[file_data_id]}\n')
        if not file_data_id in old_files:
          print(f'Added Filename: {file_data_id}, {new_files[file_data_id]}')
        elif old_files[file_data_id] != new_files[file_data_id]:
          print(f'Fixed Filename: {file_data_id}, {old_files[file_data_id]} -> {new_files[file_data_id]}')
      else:
        f.write(f'{file_data_id}, {old_files[file_data_id]}\n')

if __name__ == '__main__':
  if len(sys.argv) < 2 or len(sys.argv) > 3:
    print(f'Usage: {sys.argv[0]} binary_path [dbfile]', file=sys.stderr)
    exit(1)

  try:
    dbfile = sys.argv[2]
  except IndexError:
    dbfile = 'dbfile'

  update_dbfile(sys.argv[1], dbfile)
