import optparse, time, sys

import build_cfg, casc, jenkins

parser = optparse.OptionParser( usage = 'Usage: %prog -d wow_install_dir [options] file_path ...')
parser.add_option( '-m', '--mode', dest = 'mode', choices = [ 'dbc', 'unpack', 'extract' ],
                   help = 'Extraction mode: "root" for root/encoding file extraction, "dbc" for DBC file extraction' )
parser.add_option( '-b', '--dbfile', dest = 'dbfile', type = 'string', default = 'dbfile', 
					help = "A textual file containing a list of file paths to extract [default dbfile]" )
parser.add_option( '-r', '--root', dest = 'root_file', type = 'string', default = 'root',
                   help = 'Root file path location.' )
parser.add_option( '-e', '--encoding', dest = 'encoding_file', type = 'string', default = 'encoding',
				   help = 'Encoding file path location.' )
parser.add_option( '-d', '--datadir', dest = 'data_dir', type = 'string',
				   help = 'World of Warcraft install directory' )
parser.add_option( '-o', '--output', type = 'string', dest = 'output',
				   help = "Output directory for dbc mode" )

if __name__ == '__main__':
	(opts, args) = parser.parse_args()
	opts.parser = parser

	if opts.mode == 'dbc':
		if not opts.output:
			parser.error("DBC mode requires an output directory for the files")
			sys.exit(1)
			
		build = build_cfg.BuildCfg(opts)
		if not build.open():
			sys.exit(1)
		
		fname_db = build_cfg.DBFileList(opts)
		if not fname_db.open():
			sys.exit(1)
		
		encoding = casc.CASCEncodingFile(opts, build)
		if not encoding.open():
			sys.exit(1)
		
		root = casc.CASCRootFile(opts, build)
		if not root.open():
			sys.exit(1)
		
		index = casc.CASCDataIndex(opts)
		if not index.open():
			sys.exit(1)

		blte = casc.BLTEExtract(opts)
	
		for file_hash, file_name in fname_db.iteritems():
			extract_data = []
			
			file_md5s = root.GetFileHashMD5(file_hash)
			file_keys = []
			for md5s in file_md5s:
				file_keys = encoding.GetFileKeys(md5s)

				file_locations = []
				for file_key in file_keys:
					location = index.GetIndexData(file_key)
					if location[0] != -1:
						extract_data.append((file_key, md5s, file_name, location[0], location[1], location[2]))
			
			if len(extract_data) == 0:
				continue
			
			if len(extract_data) > 1:
				parser.error('File %s has multiple extraction locations' % file_name)
			
			print 'Extracting %s ...' % file_name
		
			if not blte.extract_file(*extract_data[0]):
				sys.exit(1)
		
	elif opts.mode == 'unpack':
		blte = casc.BLTEExtract(opts)
		for file in args:
			print 'Extracting %s ...'
			if not blte.extract_file(file):
				sys.exit(1)
	
	elif opts.mode == 'extract':
		build = build_cfg.BuildCfg(opts)
		if not build.open():
			sys.exit(1)
		
		fname_db = build_cfg.DBFileList(opts)
		if not fname_db.open():
			sys.exit(1)
		
		encoding = casc.CASCEncodingFile(opts, build)
		if not encoding.open():
			sys.exit(1)
		
		index = casc.CASCDataIndex(opts)
		if not index.open():
			sys.exit(1)
		
		keys = encoding.GetFileKeys(args[0].decode('hex'))
		if len(keys) == 0:
			parser.error('No file found with md5sum %s' % args[0])
		
		if len(keys) > 1:
			parser.error('Found multiple file keys with md5sum %s' % args[0])
		
		file_location = index.GetIndexData(keys[0])
		
		
		blte = casc.BLTEExtract(opts)
		
		if not blte.extract_file(keys[0], args[0].decode('hex'), *file_location):
			sys.exit(1)
