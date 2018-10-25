#!/usr/bin/env python3
# vim: tabstop=4 shiftwidth=4 softtabstop=4
import optparse, sys, os

import build_cfg, casc
import binascii

parser = optparse.OptionParser( usage = 'Usage: %prog -d wow_install_dir [options] file_path ...')
parser.add_option( '--cdn', dest = 'online', action = 'store_true', help = 'Fetch data from Blizzard CDN [only used for mode=batch/extract]' )
parser.add_option( '-m', '--mode', dest = 'mode', choices = [ 'batch', 'unpack', 'extract', 'fieldlist' ],
		help = 'Extraction mode: "batch" for file extraction, "unpack" for BLTE file unpack, "extract" for key or MD5 based file extract from local game client files' )
parser.add_option( '-b', '--dbfile', dest = 'dbfile', type = 'string', default = 'dbfile',
		help = "A textual file containing a list of file paths to extract [default dbfile, only needed for mode=batch]" )
parser.add_option( '-r', '--root', dest = 'root_file', type = 'string', default = 'root',
		help = 'Root file path location [default CACHE_DIR/root, only needed if --cdn is not set]' )
parser.add_option( '-e', '--encoding', dest = 'encoding_file', type = 'string', default = 'encoding',
		help = 'Encoding file path location [default CACHE_DIR/encoding, only needed if --cdn is not set]' )
parser.add_option( '-d', '--datadir', dest = 'data_dir', type = 'string',
		help = 'World of Warcraft install directory [only needed if --cdn is not set]' )
parser.add_option( '-o', '--output', type = 'string', dest = 'output',
		help = "Output directory for dbc mode, output file name for unpack mode" )
parser.add_option( '-x', '--cache', type = 'string', dest = 'cache', default = 'cache', help = 'Cache directory [default cache]' )
parser.add_option( '--ptr', action = 'store_true', dest = 'ptr', default = False, help = 'Download PTR files [default no, only used for --cdn]' )
parser.add_option( '--beta', action = 'store_true', dest = 'beta', default = False, help = 'Download Beta files [default no, only used for --cdn]' )
parser.add_option( '--alpha', action = 'store_true', dest = 'alpha', default = False, help = 'Download Alpha files [default no, only used for --cdn]' )
parser.add_option( '--classic', action = 'store_true', dest = 'classic', default = False, help = 'Download Classic files [default no, only used for --cdn]' )
parser.add_option( '--locale', action = 'store', dest = 'locale', default = 'en_US', help = 'Extraction locale [default en_US, only used for --cdn]' )
parser.add_option( '--ribbit', action = 'store_true', dest = 'ribbit', default = False, help = 'Use Ribbit for configuration information')

if __name__ == '__main__':
	(opts, args) = parser.parse_args()
	opts.parser = parser

	if not opts.mode and opts.online:
		if opts.ribbit:
			cdn = casc.RibbitIndex(opts)
		else:
			cdn = casc.CDNIndex(opts)
		cdn.CheckVersion()
		sys.exit(0)
	#elif opts.mode == 'fieldlist':
	#	build = build_cfg.BuildCfg(opts)
	#	if not build.open():
	#		sys.exit(1)

	#	bin = pe.Pe32Parser(opts)
	#	if not bin.open():
	#		sys.exit(1)
	#
	#	bin.parse()
	#	bin.generate()

	elif opts.mode == 'batch':
		if not opts.output:
			parser.error("Batch mode requires an output directory for the files")

		fname_db = build_cfg.DBFileList(opts)
		if not fname_db.open():
			sys.exit(1)

		blte = casc.BLTEExtract(opts)

		if not opts.online:
			build = build_cfg.BuildCfg(opts)
			if not build.open():
				sys.exit(1)

			encoding = casc.CASCEncodingFile(opts, build)
			if not encoding.open():
				sys.exit(1)

			index = casc.CASCDataIndex(opts)
			if not index.open():
				sys.exit(1)

			root = casc.CASCRootFile(opts, build, encoding, index)
			if not root.open():
				sys.exit(1)

			for file_hash, file_name in fname_db.items():
				extract_data = None

				file_md5s = root.GetFileHashMD5(file_hash)
				file_keys = []
				for md5s in file_md5s:
					file_keys = encoding.GetFileKeys(md5s)

					file_locations = []
					for file_key in file_keys:
						file_location = index.GetIndexData(file_key)
						if file_location[0] > -1:
							extract_data = (file_key, md5s, file_name.replace('\\', '/')) + file_location
							break

				if not extract_data:
					continue

				print('Extracting %s ...' % file_name)

				if not blte.extract_file(*extract_data):
					sys.exit(1)
		else:
			cdn = casc.CDNIndex(opts)
			if not cdn.open():
				sys.exit(1)

			encoding = casc.CASCEncodingFile(opts, cdn)
			if not encoding.open():
				sys.exit(1)

			root = casc.CASCRootFile(opts, cdn, encoding, None)
			if not root.open():
				sys.exit(1)

			output_path = os.path.join(opts.output, cdn.build())
			for file_hash, file_name in fname_db.items():
				file_md5s = root.GetFileHashMD5(file_hash)
				if not file_md5s:
					continue

				if len(file_md5s) > 1:
					print('Duplicate files found (%d) for %s, selecting first one ...' % (len(file_md5s), file_name))

				file_keys = encoding.GetFileKeys(file_md5s[0])

				if len(file_keys) == 0:
					continue

				if len(file_keys) > 1:
					print('More than one key found for %s, selecting first one ...' % file_name)

				print('Extracting %s ...' % file_name)

				data = cdn.fetch_file(file_keys[0])
				if not data:
					print('No data for a given key %s' % file_keys[0].encode('hex'))
					continue

				blte.extract_buffer_to_file(data, os.path.join(output_path, file_name.replace('\\', '/')))

	elif opts.mode == 'unpack':
		blte = casc.BLTEExtract(opts)
		for file in args:
			print('Extracting %s ...')
			if not blte.extract_file(file):
				sys.exit(1)

	elif opts.mode == 'extract':
		build = None
		index = None
		if not opts.online:
			build = build_cfg.BuildCfg(opts)
			if not build.open():
				sys.exit(1)

			index = casc.CASCDataIndex(opts)
			if not index.open():
				sys.exit(1)
		else:
			build = casc.CDNIndex(opts)
			if not build.open():
				sys.exit(1)

		encoding = casc.CASCEncodingFile(opts, build)
		if not encoding.open():
			sys.exit(1)

		root = casc.CASCRootFile(opts, build, encoding, index)
		if not root.open():
			sys.exit(1)

		keys = []
		md5s = None
		if 'key:' in args[0]:
			keys.append(binascii.unhexlify(args[0][4:]))
			file_name = args[0][4:]
		elif 'md5:' in args[0]:
			md5s = args[0][4:]
			keys = encoding.GetFileKeys(binascii.unhexlify(args[0][4:]))
			if len(keys) == 0:
				parser.error('No file found with md5sum %s' % args[0][4:])
			else:
				file_name = binascii.hexlify(keys[0]).decode('utf-8')
		else:
			file_md5s = root.GetFileMD5(args[0])
			if len(file_md5s) == 0:
				parser.error('No file named %s found' % args[0])

			keys = encoding.GetFileKeys(file_md5s[0])
			file_name = args[0]
			#print args[0], len(file_md5s) and file_md5s[0].encode('hex') or 0, len(keys)
			#sys.exit(0)

		if len(keys) == 0:
			parser.error('No encoding information found for %s' % args[0])

		if len(keys) > 1:
			parser.error('Found multiple file keys with %s' % args[0])

		blte = casc.BLTEExtract(opts)
		if not opts.online:
			file_location = index.GetIndexData(keys[0])
			if file_location[0] == -1:
				parser.error('No file location found for %s' % args[0])


			if not blte.extract_file(keys[0], md5s and md5s.decode('hex') or None, None, *file_location):
				sys.exit(1)
		else:
			data = build.fetch_file(keys[0])
			if not data:
				print('No data for a given key %s' % keys[0].encode('hex'))
				sys.exit(1)

			output_path = os.path.join(opts.output, build.build())
			blte.extract_buffer_to_file(data, os.path.join(output_path, file_name.replace('\\', '/')))



