BASE_URL = "http://dist.blizzard.com.edgesuite.net/tpr/wow/config"

import ConfigParser, os, glob, sys, StringIO, collections, urllib2

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
		self.build_cfg_file = None
		self.cdn_domain = None
		self.cdn_dir = None
	
	def open(self):
		if not self.options.data_dir:
			self.options.parser.error('No World of Warcraft installation directory given')

		if not os.access(self.options.data_dir, os.R_OK):
			self.options.parser.error('World of Warcraft installation directory %s is not readable' % self.options.data_dir)
		
		build_file = os.path.join(self.options.data_dir, '.build.info')
		if not os.access(build_file, os.R_OK):
			self.options.parser.error('World of Warcraft installation directory does not contain a readable .build.info file')
		
		build_lines = None
		with open(build_file, 'r') as build:
			build_lines = build.readlines()
		
		if len(build_lines) != 2:
			self.options.parser.error('Unknown file format for .build.info file, expected 2 lines, got %u' % len(build_lines))
		
		line_split = build_lines[1].strip().split('|')
		if len(line_split) != 12:
			self.options.parser.error('Unknown file format for .build.info file, expected 12 fields, got %u' % len(line_split))
		
		self.build_cfg_file = line_split[2]
		self.cdn_domain = line_split[7]
		self.cdn_dir = line_split[6]
	
		if len(self.build_cfg_file) == 0:
			self.options.parser.error('Could not deduce build configuration from .build.info file')
		
		data_dir = os.path.join(self.options.data_dir, 'Data')
		if not os.access(data_dir, os.R_OK):
			self.options.parser.error('World of Warcraft installation directory does not contain a Data" directory')
			return False
		
		build_cfg_path = os.path.join(data_dir, 'config', self.build_cfg_file[0:2], self.build_cfg_file[2:4], self.build_cfg_file)
		if not os.access(build_cfg_path, os.R_OK):
			self.options.parser.error('Could not read configuration file %s' % build_cfg_path)
			return False

		self.cfg = ConfigParser.SafeConfigParser()
		# Slight hack to get the configuration file read easily
		conf_str = '[base]\n' + open(build_cfg_path, 'r').read()
		conf_str_fp = StringIO.StringIO(conf_str)
		self.cfg.readfp(conf_str_fp)
		
		print 'Wow build: %s' % self.cfg.get('base', 'build-name')
		
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
		
		return 'http://%s%s/data/%s/%s/%s' % (self.cdn_domain, self.cdn_dir, blte_file[0:2], blte_file[2:4], blte_file)
	
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
		
