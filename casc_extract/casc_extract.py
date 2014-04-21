import optparse, time

import build_cfg, casc, jenkins

parser = optparse.OptionParser()
parser.add_option( '-m', '--mode', dest = 'mode', choices = [ 'meta', 'dbc' ], default = 'meta',
                   help = 'Extraction mode: "root" for root/encoding file extraction, "dbc" for DBC file extraction' )
parser.add_option( '-r', '--root', dest = 'root_file', type = 'string',
                   help = 'Root file path location. Required for --mode=dbc' )
parser.add_option( '-e', '--encoding', dest = 'encoding_file', type = 'string',
				   help = 'Encoding file path location. Required for --mode=dbc' )
parser.add_option( '-d', '--datadir', dest = 'data_dir', type = 'string',
				   help = 'World of Warcraft install directory' )

if __name__ == '__main__':
	(opts, args) = parser.parse_args()
	
	build_cfg = build_cfg.BuildCfg(opts)
	if not build_cfg.open():
		sys.exit(1)

	if opts.mode == 'meta':
		#extractor = casc.BLTEExtract(opts, args)
		#files = [ build_cfg.root_file(), build_cfg.encoding_file() ]
		#print 'Extracting root/encoding file(s) %s' % ', '.join(files)
		#extractor.extract_file_by_md5(files)

		encoding = casc.CASCEncodingFile(opts, build_cfg.encoding_file())
		encoding.open()
		
		root = casc.CASCRootFile(opts, encoding, build_cfg.root_file())
		root.open()
		
		index = casc.CASCDataIndex(opts)
		index.open()
		
		md5ss = root.GetFileMD5('DBFilesClient\\SpellClassOptions.db2')
		#print encoding.GetFileKeys('c94630ca72f95de6716464f28e62cade'.decode('hex'))
		for md5s in md5ss:
			keys = encoding.GetFileKeys(md5s)
			print md5s.encode('hex'), keys[0].encode('hex')
			print index.GetIndexData(keys[0])
		
		#time.sleep(1000000)