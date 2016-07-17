import struct, json, pathlib, sys, re

class DBFormat(object):
    def __find_newest_file(self, path):
        valid_files = []
        for f in path.iterdir():
            mobj = re.search("([0-9]+)", f.stem)
            if mobj:
                valid_files.append((mobj.group(1), f))

        valid_files.sort()
        return valid_files[-1][1]

    def __find_format_file(self):
        if not self.options.format:
            return self.__find_newest_file(pathlib.Path('formats'))
        else:
            p = pathlib.Path(self.options.format)
            if not p.exists():
                sys.stderr.write('File "%s" does not exist\n' % p)
                sys.exit(1)

            if p.is_dir():
                return self.__find_newest_file(p)
            else:
                return p

    def __init__(self, opts):
        self.options = opts
        self.data = {}

        self.__do_init()

    def __do_init(self):
        js = json.load(self.__find_format_file().open())

        for dbcfile, data in js.items():
            for field_conf in data:
                if dbcfile not in self.data:
                    self.data[dbcfile] = {
                        'data-format': [],
                        'data-fields': [],
                        'cpp'        : [],
                        'id-format'  : '%u',
                    }

                fmt = field_conf.get('data_type', u'I')
                outfmt = u'%u'
                if fmt in 'ihb':
                    outfmt = u'%d'
                elif fmt in 'f':
                    outfmt = u'%f'
                elif fmt == 'S':
                    outfmt = u'%s'
                self.data[dbcfile]['data-format'].append(fmt)
                self.data[dbcfile]['data-fields'].append(field_conf.get('field', None))
                self.data[dbcfile]['cpp'        ].append(field_conf.get('formats', {}).get('cpp', outfmt))

            self.data[dbcfile]['parser'] = struct.Struct('=' + ''.join(self.data[dbcfile]['data-format']).replace(u'S', u'I'))

    def set_id_format(self, file_name, formatstr):
        if file_name not in self.data:
            raise Exception('Unable to find data format for %s' % file_name)

        self.data[file_name]['id-format'] = formatstr

    def formats(self, file_name, formatstr):
        if file_name not in self.data:
            raise Exception('Unable to find data format for %s' % file_name)

        return self.data[file_name][formatstr]

    def types(self, file_name):
        if file_name not in self.data:
            raise Exception('Unable to find data format for %s' % file_name)

        return self.data[file_name]['data-format']

    def fields(self, file_name):
        if file_name not in self.data:
            raise Exception('Unable to find data format for %s' % file_name)

        return self.data[file_name]['data-fields']

    def parser(self, file_name):
        if file_name not in self.data:
            raise Exception('Unable to find data format for %s' % file_name)

        return self.data[file_name]['parser']

    def id_format(self, file_name):
        if file_name not in self.data:
            raise Exception('Unable to find data format for %s' % file_name)

        return self.data[file_name]['id-format']

