# vim: tabstop=4 shiftwidth=4 softtabstop=4
import json, pathlib, binascii

__KEYFILE = None

class KeyFile:
	def __init__(self, options):
		self.options = options
	
	def open(self):
		if not self.options.key_file:
			return True

		f = pathlib.Path(self.options.key_file)
		if not f.exists() or not f.is_file():
			return False

		try:
			data = json.load(open(self.options.key_file, 'r'))
		except Exception as e:
			return False

		if not isinstance(data, list):
			return False

		self.keys = {}
		for entry in data:
			if not entry['key']:
				continue

			self.keys[entry['key_id']] = bytes.fromhex(entry['key'])

		return True

	def find(self, name):
		key_name = binascii.hexlify(name).decode('ascii')

		return self.keys.get(key_name, None)

def initialize(opts):
	global __KEYFILE

	__KEYFILE = KeyFile(opts)

	return __KEYFILE.open()

def find(name):
	return __KEYFILE.find(name)
