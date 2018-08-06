import configparser, sys, importlib, logging, types, os

import dbc.db

class Config:
    def __init__(self, options):
        self.options = options
        self.data_store = dbc.db.DataStore(self.options)

        self.config = {}
        self.base_module_path = None
        self.base_module = None
        self.base_output_path = None

    def output_file(self, file_name):
        return os.path.join(self.base_output_path, file_name)

    def open(self):
        if len(self.options.args) < 1:
            return False

        config = configparser.ConfigParser()
        config.read(self.options.args[0])

        for section in config.sections():
            if section == 'general':
                self.base_module_path = config.get('general', 'module_base')
                self.base_output_path = config.get('general', 'output_base')
            else:
                if section not in self.config:
                    self.config[section] = { 'generators': [], 'objects': [], 'append': False }

                for i in config.get(section, 'generators').split():
                    self.config[section]['generators'].append(i)

                self.config[section]['append'] = config.get(section, 'append', fallback = False)

        if not self.base_module_path:
            logging.error('No "module_base" defined in general section')
            return False

        if not self.base_output_path:
            logging.error('No "output_base" defined in general section')
            return False

        if not os.access(self.base_output_path, os.W_OK):
            logging.error('Cannot output to "%s", directory not writable', self.base_output_path)
            return False

        try:
            self.base_module = importlib.import_module(self.base_module_path)
        except:
            logging.error('Unable to import %s', self.base_module_path)
            return False

        return True

    def generate(self):
        for section, config in self.config.items():
            for generator in config['generators']:
                try:
                    obj = getattr(self.base_module, generator)
                except AttributeError:
                    logging.error('Unable to instantiate generator %s', generator)
                    return False

                config['objects'].append(obj(self.options, self.data_store))

        for section, config in self.config.items():
            for generator in config['objects']:
                logging.info('Initializing %s ...', generator.__class__.__name__)
                if not generator.initialize():
                    logging.error('Unable to initialize %s, exiting ...', generator.__class__.__name__)
                    return False

        for section, config in self.config.items():
            output_file = self.output_file(section)

            for generator in config['objects']:
                is_append = generator != config['objects'][0] or config.get('append', False)
                if not generator.set_output(output_file, is_append):
                    return False

                ids = generator.filter()

                logging.info('Outputting %s to %s ...', generator.__class__.__name__, output_file)
                generator.generate(ids)
                generator.close()

