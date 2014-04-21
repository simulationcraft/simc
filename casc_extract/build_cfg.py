import ConfigParser, os, glob, sys, StringIO

class BuildCfg(object):
	def __init__(self, options):
		self.options = options
		self.cfg = None
	
	def open(self):
		if not os.access(self.options.data_dir, os.R_OK):
			print >>sys.stderr, 'World of Warcraft installation directory %s is not readable' % self.options.data_dir
			return False
		
		build_file = os.path.join(self.options.data_dir, '.build.db')
		if not os.access(build_file, os.R_OK):
			print >>sys.stderr, 'World of Warcraft installation directory does not contain a readable .build.db file'
			return False
		
		build_str = ''
		with open(build_file, 'r') as build:
			build_str = build.read().strip().split('|')[0]
		
		if len(build_str) == 0:
			print >>sys.stderr, 'Could not deduce build configuration from .build.db file'
			return False
		
		data_dir = os.path.join(self.options.data_dir, 'Data')
		if not os.access(data_dir, os.R_OK):
			print >>sys.stderr, 'World of Warcraft installation directory does not contain a Data" directory'
			return False
		
		build_cfg_file = os.path.join(data_dir, 'config', build_str[0:2], build_str[2:4], build_str)
		if not os.access(data_dir, os.R_OK):
			print >>sys.stderr, 'Could not read configuration file %s' % build_cfg_file
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
			return False
		
		if not self.cfg.has_option('base', 'root'):
			print >>sys.stderr, 'Build configuration has no "root" option'
			return None
		
		v = self.cfg.get('base', 'root').split(' ')
		if type(v) == list:
			return v[0]
		else:
			return v
	
	def encoding_file(self):
		if not self.cfg and not open():
			return False
		
		if not self.cfg.has_option('base', 'root'):
			print >>sys.stderr, 'Build configuration has no "encoding" option'
			return None
		
		v = self.cfg.get('base', 'encoding').split(' ')
		if type(v) == list:
			return v[0]
		else:
			v