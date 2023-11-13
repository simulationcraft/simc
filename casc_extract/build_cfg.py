# vim: tabstop=4 shiftwidth=4 softtabstop=4

import configparser, os, glob, sys, io, collections, csv

import jenkins

def product_arg_str(opts):
	if opts.product:
		return opts.product
	elif opts.ptr:
		return 'wowt'
	elif opts.beta:
		return 'wow_beta'
	elif opts.alpha:
		return 'wow_alpha'
	elif opts.classic:
		return 'wow_classic'
	else:
		return 'wow'

class DBFileList(collections.abc.Mapping):
	def __init__(self, options):
		collections.abc.Mapping.__init__(self)

		self.options = options

		self.files = {}

	def __getitem__(self, key):
		return self.files.__getitem__(key)

	def __iter__(self):
		return self.files.__iter__()

	def __len__(self):
		return self.files.__len__()

	def open(self):
		if not self.options.dbfile:
			self.options.parser.error('No filename list given')
			return False

		if not os.access(self.options.dbfile, os.R_OK):
			self.options.parser.error(f'Unable to open filename list {self.options.dbfile}')
			return False

		with open(self.options.dbfile, 'r') as f:
			for line in f:
				cleaned_file = line.strip().replace('/', '\\')
				if len(cleaned_file) == 0:
					continue

				split_line = cleaned_file.split(',', 1)
				if len(split_line) != 2:
					self.options.parser.error(
						f'Invalid dbfile line "{cleaned_file}", lines require a file data id and a file name, separated by commas'
					)

				file_data_id = int(split_line[0].strip())
				file_name = split_line[1].strip()

				#hash_value = jenkins.hashlittle2(file_name.upper())
				#self.files[(hash_value[0] << 32) | hash_value[1]] = (file_name, file_data_id)
				self.files[file_data_id] = file_name

				#base_fname = os.path.splitext(file_name)[0]
				#for ext in [ '.db2', '.dbc' ]:
				#	final_file = base_fname + ext
				#	hash_value = jenkins.hashlittle2(final_file.upper())

				#	self.files[(hash_value[0] << 32) | hash_value[1]] = (final_file, file_data_id)

		return True

class BuildCfg(object):
	BI_ACTIVE = 'Active!DEC:1'
	BI_BUILD_KEY = 'Build Key!HEX:16'
	BI_CDN_PATH = 'CDN Path!STRING:0'
	BI_CDN_HOSTS = 'CDN Hosts!STRING:0'
	BI_VERSION = 'Version!STRING:0'
	BI_PRODUCT = 'Product!STRING:0'

	def __init__(self, options):
		self.options = options
		self.cfg = None
		self.build_cfg_file = None
		self.cdn_domain = None
		self.cdn_dir = None

	def open(self):
		if not self.options.data_dir:
			self.options.parser.error('No World of Warcraft installation directory given')

		if not os.access(self.options.data_dir, os.R_OK):
			self.options.parser.error(
				f'World of Warcraft installation directory {self.options.data_dir} is not readable'
			)

		build_file = os.path.join(self.options.data_dir, '.build.info')
		if not os.access(build_file, os.R_OK):
			self.options.parser.error(
				'World of Warcraft installation directory does not contain a readable .build.info file'
			)

		build_version = None
		with open(build_file, 'r') as build:
			try:
				reader = csv.DictReader(build, delimiter='|')
			except csv.Error as err:
				self.options.parser.error(f'Unable to parse .build_info, error: {str(err)}')

			req_fields = [
				BuildCfg.BI_ACTIVE,
				BuildCfg.BI_BUILD_KEY,
				BuildCfg.BI_CDN_PATH,
				BuildCfg.BI_CDN_HOSTS,
				BuildCfg.BI_VERSION,
				BuildCfg.BI_PRODUCT
			]

			for field in req_fields:
				if field not in reader.fieldnames:
					self.options.parser.error(
						f'No field named {field} found in .build_info, available fields: {", ".join(reader.fieldnames)}'
					)

			for line in reader:
				if line[BuildCfg.BI_PRODUCT] != product_arg_str(self.options):
					continue

				if int(line[BuildCfg.BI_ACTIVE]) == 0:
					print(f'Warning, build configuration {product_arg_str(self.options)} marked inactive',
						file=sys.stderr)

				self.build_cfg_file = line[BuildCfg.BI_BUILD_KEY]
				self.cdn_domain = line[BuildCfg.BI_CDN_HOSTS].split(' ')[0].strip()
				self.cdn_dir = line[BuildCfg.BI_CDN_PATH]
				build_version = line[BuildCfg.BI_VERSION]
				break

		if len(self.build_cfg_file) == 0:
			self.options.parser.error('Could not deduce build configuration from .build.info file')

		data_dir = os.path.join(self.options.data_dir, 'Data')
		if not os.access(data_dir, os.R_OK):
			self.options.parser.error(
				'World of Warcraft installation directory does not contain a "Data" directory'
			)

		build_cfg_path = os.path.join(data_dir, 'config', self.build_cfg_file[0:2],
			self.build_cfg_file[2:4], self.build_cfg_file)
		if not os.access(build_cfg_path, os.R_OK):
			self.options.parser.error(f'Could not read configuration file {build_cfg_path}')

		self.cfg = configparser.ConfigParser()
		# Slight hack to get the configuration file read easily
		conf_str = '[base]\n' + open(build_cfg_path, 'r').read()
		conf_str_fp = io.StringIO(conf_str)
		self.cfg.readfp(conf_str_fp)

		print(f'Wow build: {build_version}')

		return True

	def root_file(self):
		if not self.cfg and not open():
			return None

		if not self.cfg.has_option('base', 'root'):
			self.options.parser.error('Build configuration has no "root" option')
			return None

		v = self.cfg.get('base', 'root').split(' ')
		if type(v) == list:
			return v[0]
		else:
			return v

	def encoding_file(self):
		if not self.cfg and not open():
			return False

		if not self.cfg.has_option('base', 'encoding'):
			self.options.parser.error('Build configuration has no "encoding" option')
			return None

		v = self.cfg.get('base', 'encoding').split(' ')
		if type(v) == list:
			return v[0]
		else:
			v

	def encoding_blte_url(self):
		blte_file = self.encoding_blte()

		return 'http://%s/%s/data/%s/%s/%s' % (self.cdn_domain, self.cdn_dir, blte_file[0:2], blte_file[2:4], blte_file)

	def cdn_url(self, file_name):
		return 'http://%s%s/data/%s/%s/%s' % (self.cdn_domain, self.cdn_dir, file_name[0:2], file_name[2:4], file_name)

	def encoding_blte(self):
		if not self.cfg and not open():
			return None

		if not self.cfg.has_option('base', 'encoding'):
			self.options.parser.error('Build configuration has no "encoding" option')
			return None

		v = self.cfg.get('base', 'encoding').split(' ')
		if type(v) == list and len(v) == 2:
			return v[1]
		else:
			self.options.parser.error('Unable to find BLTE signature for "encoding" file')
			return False

