#! /usr/bin/env python
# encoding: utf-8

import os, Logs, Options

top = '.'
out = 'build'
APPNAME='gmidimonitor'
VERSION='3.6'

optfeatures = {
    'jack': {'description': 'JACK MIDI support',
             'site': 'http://jackaudio.org/'},
    'alsa': {'description': 'ALSA MIDI support',
             'site': 'http://www.alsa-project.org/'},
    'lash': {'description': 'LASH support',
             'site': 'http://www.nongnu.org/lash/',
             'package': 'lash-1.0'},
    }

def display_line(text = ""):
    Logs.pprint('NORMAL', text)

def display_value(ctx, text, value, color = 'NORMAL'):
    ctx.msg(text, value, color)

def display_feature(ctx, opt):
    if optfeatures[opt]['build']:
        color = 'GREEN'
    else:
        color = 'YELLOW'

    display_value(ctx, optfeatures[opt]['description'], optfeatures[opt]['status'], color)

def add_cflag(ctx, flag):
    ctx.env.append_unique('CXXFLAGS', flag)
    ctx.env.append_unique('CFLAGS', flag)

def add_linkflag(ctx, flag):
    ctx.env.append_unique('LINKFLAGS', flag)

def add_opt_feature(opt, feature, help):
    help = "Whether to build " + help + ", possible values: 'yes', 'no' and 'auto'. Default is 'auto'."
    opt.add_option('--' + feature, type='choice', choices=['auto', 'yes', 'no'], default='auto', help=help)

def options(opt):
    opt.load('compiler_c')
    for k, v in optfeatures.items():
        add_opt_feature(opt, k, v['description'])

def check_opt_packages(ctx):
    for opt, v in optfeatures.items():
        site = v['site']
        try:
            package = v['package']
        except KeyError:
            package = opt

        value = getattr(Options.options, opt)
        if value == 'no':
            optfeatures[opt]['build'] = False
            optfeatures[opt]['status'] = 'disabled (requested)'
        else:
            mandatory = value == 'yes'
            if mandatory:
                errmsg = 'not found, check ' + site
            else:
                errmsg = 'not found'
            optfeatures[opt]['build'] = bool(ctx.check_cfg(package = package, mandatory = mandatory, args = '--cflags --libs', errmsg = errmsg))
            if optfeatures[opt]['build']:
                if mandatory:
                    optfeatures[opt]['status'] = 'enabled (requested)'
                else:
                    optfeatures[opt]['status'] = 'enabled (auto)'
            else:
                optfeatures[opt]['status'] = 'disabled, package ' + package + ' not found, check ' + site

        ctx.env['BUILD_' + opt.upper()] = optfeatures[opt]['build']

def configure(ctx):
    ctx.load('compiler_c')

    ctx.check_cfg(
        package = 'gtk+-2.0',
        errmsg = "not installed, see http://www.gtk.org/",
        args = '--cflags --libs')

    ctx.check_cfg(
        package = 'gthread-2.0',
        errmsg = "not installed, see http://www.gtk.org/",
        args = '--cflags --libs')

    ctx.check_cfg(
        package = 'gmodule-2.0',
        errmsg = "not installed, see http://www.gtk.org/",
        args = '--cflags --libs')

    check_opt_packages(ctx)

    if not optfeatures['jack']['build'] and not optfeatures['alsa']['build']:
        display_line()
        display_feature(ctx, 'alsa')
        display_feature(ctx, 'jack')
        ctx.fatal("Neither JACK nor ALSA is available")

    ctx.env['DATA_DIR'] = os.path.normpath(os.path.join(ctx.env['PREFIX'], 'share', APPNAME))

    add_cflag(ctx, '-Wall')
    #add_cflag(ctx, '-Wuninitialized')
    #add_cflag(ctx, '-O3')
    #add_cflag(ctx, '-g')
    #add_linkflag(ctx, '-g')

    display_line()
    display_line('==================')
    display_value(ctx, 'Prefix', ctx.env['PREFIX'])
    display_feature(ctx, 'jack')
    display_feature(ctx, 'alsa')
    display_feature(ctx, 'lash')
    display_line('==================')
    display_line()

    ctx.define('PACKAGE_VERSION', VERSION)
    ctx.define('APPNAME', APPNAME)
    ctx.define('DATA_DIR', ctx.env['DATA_DIR'])
    ctx.write_config_header('config.h')

def build(ctx):
    source = [
        "main.c",
        "about.c",
        "path.c",
        "gm.c",
        "log.c",
        "memory_atomic.c",
        "sysex.c",
        ]
    uselib = ['GTK+-2.0', 'GTHREAD-2.0', 'GMODULE-2.0']

    if ctx.env['BUILD_JACK']:
        source.append("jack.c")
        uselib.append('JACK')

    if ctx.env['BUILD_ALSA']:
        source.append("alsa.c")
        uselib.append('ALSA')

    if ctx.env['BUILD_LASH']:
        uselib.append('LASH-1.0')

    ctx.program(
        source = source,
        features = 'c cprogram',
        includes = ".",
        target = 'gmidimonitor',
        uselib = uselib,
        )

    ctx.install_files('${DATA_DIR}', 'gmidimonitor.ui')
