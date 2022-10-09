project(
    'astera-sdk-c',
    'c',
    meson_version: '>= 0.57.0',
    version: '0.1',
)
c = meson.get_compiler('c')
i2c = meson.get_compiler('c').find_library('i2c')
incdir = include_directories('examples/include', 'include')


executable('aries-sdk-c-test',
            'source/aries_api.c',
            'source/aries_i2c.c',
            'source/aries_misc.c',
            'source/astera_log.c',
            'examples/source/aspeed.c',
            'examples/aries_test.c',
            include_directories : incdir,
            implicit_include_directories: false,
            install: true,
            install_dir: get_option('bindir'),
            dependencies: i2c,
)
executable('aries-sdk-c-eeprom-test',
            'source/aries_api.c',
            'source/aries_i2c.c',
            'source/aries_misc.c',
            'source/astera_log.c',
            'examples/source/aspeed.c',
            'examples/eeprom_test.c',
            include_directories : incdir,
            implicit_include_directories: false,
            install: true,
            install_dir: get_option('bindir'),
            dependencies: i2c,
)
executable('aries-sdk-c-eeprom-update',
            'source/aries_api.c',
            'source/aries_i2c.c',
            'source/aries_misc.c',
            'source/astera_log.c',
            'examples/source/aspeed.c',
            'examples/eeprom_update.c',
            include_directories : incdir,
            implicit_include_directories: false,
            install: true,
            install_dir: get_option('bindir'),
            dependencies: i2c,
)