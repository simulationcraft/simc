import struct, json, pathlib, sys, re, logging

import dbc

class Field:
    def __init__(self, index, data):
        self.index = index
        self.data_type = data.get('data_type', u'I')
        self.output_format = data.get('formats', {}).get('cpp', None)
        self.name = data.get('field')
        self.elements = data.get('elements', 1)
        self.block = data.get('block', False)

    def data_types(self):
        return [ self.data_type, ] * self.elements

    def index(self):
        return self.index

    def type_name(self):
        if self.data_type == 'Q':
            return 'uint64'
        elif self.data_type == 'q':
            return 'int64'
        elif self.data_type == 'I':
            return 'uint32'
        elif self.data_type == 'i':
            return 'int32'
        elif self.data_type == 'H':
            return 'uint16'
        elif self.data_type == 'h':
            return 'int16'
        elif self.data_type == 'B':
            return 'uint8'
        elif self.data_type == 'b':
            return 'int8'
        elif self.data_type == 'f':
            return 'float'
        elif self.data_type == 'S':
            return 'string'

        return 'unknown'

    def output_formats(self):
        outfmt = self.output_format

        if not self.output_format:
            if self.data_type in 'ihb':
                outfmt = u'%d'
            elif self.data_type in 'f':
                outfmt = u'%f'
            elif self.data_type == 'S':
                outfmt = u'%s'
            elif self.data_type == 'Q':
                outfmt = u'%uLLU'
            elif self.data_type == 'q':
                outfmt = u'%dLLD'
            else:
                outfmt = u'%u'

        return [ outfmt, ] * self.elements

    def base_name(self):
        return self.name

    def field_names(self):
        if self.elements == 1:
            return [ self.name, ]

        v = []
        for i in range(0, self.elements):
            v.append('%s_%d' % (self.name, i + 1))

        return v

    def struct_types(self):
        return [ self.data_type.replace('S', 'I'), ] * self.elements

    def block_field(self):
        return self.block

    def field_size(self):
        if self.data_type in 'bB':
            return ( 1, )
        elif self.data_type in 'hH':
            return ( 2, )
        elif self.data_type in 'iI':
            return ( 3, 4 )
        elif self.data_type in 'S':
            return ( 1, 2, 3, 4 )
        elif self.data_type in 'f':
            return ( 4, )
        elif 'x' in self.data_type:
            mobj = re.match('^([0-9]*)x$', self.data_type)
            if mobj.group(1):
                return (int(mobj.group(1)),)

            return (1,)


        return ()

    def __str__(self):
        return 'index=%u name=%s type=%c output=%s elements=%d block=%s' % (
            self.index, self.name, self.data_type, self.output_format, self.elements, self.block)

class DBFormat(object):
    def __find_newest_file(self, path):
        valid_files = []
        for f in path.iterdir():
            try:
                ver = dbc.WowVersion(f.stem)
            except:
                continue
            valid_files.append((ver, f))

        # Format file patch level should match the build patch level given
        patch_files = list(filter(lambda x: x[0].patch_level() == self.options.build.patch_level(),
            valid_files))
        # Nothing usable found for the patch level, just grab all we can and
        # use the highest version format file we can find (in relation to build option)
        if len(patch_files) == 0:
            patch_files = valid_files
        patch_files.sort()
        idx = 0

        while idx < len(patch_files) and patch_files[idx][0] <= self.options.build:
            idx += 1

        logging.debug('Opened format file %s', patch_files[idx - 1][1])
        return patch_files[idx - 1][1]

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
            field_idx = 0
            for field_conf in data:
                if dbcfile not in self.data:
                    self.data[dbcfile] = {
                        'data-format': [],
                        'data-fields': [],
                        'cpp'        : [],
                        'obj'        : []
                    }

                fmt = field_conf.get('data_type', u'I')
                outfmt = u'%u'
                if fmt in [ 'i', 'h', 'b' ]:
                    outfmt = u'%d'
                elif fmt in 'f':
                    outfmt = u'%f'
                elif fmt == 'S':
                    outfmt = u'%s'
                elif fmt == 'q':
                    outfmt = u'%dLLD'
                elif fmt == 'Q':
                    outfmt = u'%uLLU'

                # Full data format
                self.data[dbcfile]['data-format'].append(fmt)
                self.data[dbcfile]['data-fields'].append(field_conf.get('field', None))
                self.data[dbcfile]['cpp'        ].append(field_conf.get('formats', {}).get('cpp', outfmt))
                self.data[dbcfile]['obj'        ].append(Field(field_idx, field_conf))

                field_idx += 1

    def obj(self, file_name, **kwargs):
        if kwargs.hasattr('index'):
            return self.data[file_name]['obj'][kwargs.index]

    def objs(self, file_name, include_all = False):
        if file_name not in self.data:
            raise Exception('Unable to find data format for %s' % file_name)

        v = []
        for obj in self.data[file_name]['obj']:
            if include_all or (not include_all and not obj.block_field()):
                v.append(obj)

        return v

    def formats(self, file_name, include_all = False):
        if file_name not in self.data:
            raise Exception('Unable to find data format for %s' % file_name)

        v = []
        for obj in self.objs(file_name, include_all):
            if include_all or (not include_all and not obj.block_field()):
                v += obj.output_formats()

        return v

    def types(self, file_name, include_all = False):
        if file_name not in self.data:
            raise Exception('Unable to find data format for %s' % file_name)

        v = []
        for obj in self.objs(file_name, include_all):
            if include_all or (not include_all and not obj.block_field()):
                v += obj.data_types()

        return v

    def fields(self, file_name, include_all = False):
        if file_name not in self.data:
            raise Exception('Unable to find data format for %s' % file_name)

        v = []
        for obj in self.objs(file_name, include_all):
            if include_all or (not include_all and not obj.block_field()):
                v += obj.field_names()

        return v
