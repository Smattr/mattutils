def meta_x(api, argv):
    '''terminal'''
    import os
    return api['run'](os.path.expanduser('~/bin/term'))[0]

def meta_z(api, argv):
    '''browser'''
    cmd = ['chromium-browser', '--incognito']
    return api['run'](cmd + argv[1:])[0]
