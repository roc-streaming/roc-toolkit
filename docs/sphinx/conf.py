# -*- coding: utf-8 -*-

import datetime
import os
import re
import subprocess
import sys

def get_version():
    proc = subprocess.Popen(
        [sys.executable,
         os.path.dirname(os.path.realpath(__file__)) +
            '/../../scripts/scons_helpers/parse-version.py'],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT)
    return tuple(proc.stdout.read().decode().strip().split('.'))

# -- General configuration ------------------------------------------------

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.coverage',
    'sphinx.ext.mathjax',
    'sphinxemoji.sphinxemoji',
    'breathe',
    'sphinx_rtd_theme'
]

templates_path = []

source_suffix = ['.rst']
exclude_patterns = []

master_doc = 'index'

project = u'Roc Toolkit'
copyright = u'{}, Roc Streaming authors'.format(datetime.datetime.now().year)
author = u'Roc Streaming authors'

version_tuple = get_version()

version = 'Roc Toolkit {}'.format('.'.join(version_tuple[:2]))
release = '.'.join(version_tuple)

today_fmt = '%Y'

pygments_style = 'sphinx'

todo_include_todos = False

# -- Options for Breathe ----------------------------------------------

breathe_projects = { 'roc': '../../build/docs/public_api/xml' }

breathe_default_project = 'roc'
breathe_domain_by_extension = {'h': 'c'}

# -- Options for HTML output ----------------------------------------------

html_title = '{} {}'.format(project, release)

html_theme = 'sphinx_rtd_theme'

html_logo = '../images/logo80.png'

html_sidebars = {
   '**': ['globaltoc.html', 'searchbox.html'],
}
html_theme_options = {
    # Toc options
    'collapse_navigation': False,
    'sticky_navigation': False,
    'navigation_depth': -1,
    'includehidden': True,
    'titles_only': False
}


html_static_path = ['_static']

html_css_files = [
    'roc.css',
]

html_js_files = [
    'analytics.js',
]

# -- Options for manual page output ---------------------------------------

man_pages = [
    ('manuals/roc_send', 'roc-send', u'send real-time audio', [], 1),
    ('manuals/roc_recv', 'roc-recv', u'receive real-time audio', [], 1),
    ('manuals/roc_copy', 'roc-copy', u'copy local audio', [], 1),
]
