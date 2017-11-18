def meta_z(api, argv):
    '''browser'''
    cmd = ['chromium-browser', '--incognito']
    return api['run'](cmd + argv[1:])[0]
