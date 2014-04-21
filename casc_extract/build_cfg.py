import ConfigParser, os, glob, sys, StringIO, collections

import jenkins

class DBFileList(collections.Mapping):
	def __init__(self, options):
		collections.Mapping.__init__(self)
		
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
			print self.options.parser('No filename list given')
			return False
		
		if not os.access(self.options.dbfile, os.R_OK):
			print self.options.parser('Unable to open filename list %s' % self.options.dbfile)
			return False
		
		with open(self.options.dbfile, 'r') as f:
			for line in f:
				cleaned_file = line.strip().replace('/', '\\')
				base_fname = os.path.splitext(cleaned_file)[0]
				
				for ext in [ '.db2', '.dbc' ]:
					final_file = base_fname + ext
					hash_value = jenkins.hashlittle2(final_file.upper())
					
					self.files[(hash_value[0] << 32) | hash_value[1]] = final_file
		
		return True

class BuildCfg(object):
	def __init__(self, options):
		self.options = options
		self.cfg = None
	
	def open(self):
		if not self.options.data_dir:
			self.options.parser.error('No World of Warcraft installation directory given')
			return False

		if not os.access(self.options.data_dir, os.R_OK):
			self.options.parser.error('World of Warcraft installation directory %s is not readable' % self.options.data_dir)
			return False
		
		build_file = os.path.join(self.options.data_dir, '.build.db')
		if not os.access(build_file, os.R_OK):
			self.options.parser.error('World of Warcraft installation directory does not contain a readable .build.db file')
			return False
		
		build_str = ''
		with open(build_file, 'r') as build:
			build_str = build.read().strip().split('|')[0]
		
		if len(build_str) == 0:
			self.options.parser.error('Could not deduce build configuration from .build.db file')
			return False
		
		data_dir = os.path.join(self.options.data_dir, 'Data')
		if not os.access(data_dir, os.R_OK):
			self.options.parser.error('World of Warcraft installation directory does not contain a Data" directory')
			return False
		
		build_cfg_file = os.path.join(data_dir, 'config', build_str[0:2], build_str[2:4], build_str)
		if not os.access(data_dir, os.R_OK):
			self.options.parser.error('Could not read configuration file %s' % build_cfg_file)
			return False

		self.cfg = ConfigParser.SafeConfigParser()
		# Slight hack to get the configuration file read easily
		conf_str = '[base]\n' + open(build_cfg_file, 'r').read()
		conf_str_fp = StringIO.StringIO(conf_str)
		self.cfg.readfp(conf_str_fp)
		
		self.open = True
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

	def encoding_blte(self):
		if not self.cfg and not open():
			return None
		
		if not self.cfg.has_option('base', 'encoding'):
			self.options.parser.error('Build configuratino has no "encoding" option')
			return None
		
		v = self.cfg.get('base', 'encoding').split(' ')
		if type(v) == list and len(v) == 2:
			return v[1]
		else:
			self.options.parser.error('Unable to find BLTE signature for "encoding" file')
			return False
		
