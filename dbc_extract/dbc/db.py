class DBCDB(dict):
    def __init__(self, obj = None):
        dict.__init__(self)
        self.__obj = obj

    def __missing__(self, key):
        if self.__obj:
           return self.__obj.default()
        else:
            raise KeyError
