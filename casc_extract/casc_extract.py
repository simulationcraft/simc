import optparse, time, sys

import build_cfg, casc, jenkins

parser = optparse.OptionParser( usage = 'Usage: %prog -d wow_install_dir [options] file_path ...')
parser.add_option( '-m', '--mode', dest = 'mode', choices = [ 'meta', 'dbc' ],
                   help = 'Extraction mode: "root" for root/encoding file extraction, "dbc" for DBC file extraction' )
parser.add_option( '-b', '--dbfile', dest = 'dbfile', type = 'string', default = 'dbfile', 
					help = "A textual file containing a list of file paths to extract [default dbfile]" )
parser.add_option( '-r', '--root', dest = 'root_file', type = 'string', default = 'root',
                   help = 'Root file path location.' )
parser.add_option( '-e', '--encoding', dest = 'encoding_file', type = 'string', default = 'encoding',
				   help = 'Encoding file path location.' )
parser.add_option( '-d', '--datadir', dest = 'data_dir', type = 'string',
				   help = 'World of Warcraft install directory' )

if __name__ == '__main__':
	(opts, args) = parser.parse_args()
	opts.parser = parser
	
	build = build_cfg.BuildCfg(opts)
	if not build.open():
		sys.exit(1)

	if opts.mode == 'dbc':
		fname_db = build_cfg.DBFileList(opts)
		if not fname_db.open():
			sys.exit(1)

		for k, v in fname_db.iteritems():
			print k, v
			
	if opts.mode == 'meta':
		encoding = casc.CASCEncodingFile(opts, build_cfg.encoding_file())
		encoding.open()
		
		root = casc.CASCRootFile(opts, encoding, build_cfg.root_file())
		root.open()
		
		index = casc.CASCDataIndex(opts)
		index.open()
		
		md5ss = root.GetFileMD5('DBFilesClient\\SpellClassOptions.db2')

		for md5s in md5ss:
			keys = encoding.GetFileKeys(md5s)
			print md5s.encode('hex'), keys[0].encode('hex')
			print index.GetIndexData(keys[0])
		
		#time.sleep(1000000)