# -*- coding: utf-8 -*-

import datetime
import os

# -- General configuration ------------------------------------------------

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.coverage',
    'sphinx.ext.mathjax',
    'breathe',
    'rst2pdf.pdfbuilder',
]

templates_path = []

source_suffix = ['.rst']
exclude_patterns = []

master_doc = 'index'

project = u'Roc Toolkit'
copyright = u'%s, Roc authors' % datetime.datetime.now().year
author = u'Roc authors'

version_tuple = open(os.path.join(
    os.path.dirname(__file__), '../../../.version')).read().strip().split('.')

version = 'Roc Toolkit %s' % '.'.join(version_tuple[:2])
release = '%s' % '.'.join(version_tuple)

pygments_style = 'sphinx'

todo_include_todos = False

pdf_filename = u'RocToolKit'

pdf_documents = [(master_doc,pdf_filename,u'RocToolkit Documentation',author),]
# -- Options for Breathe ----------------------------------------------

breathe_projects = { 'roc': '../../../build/docs/lib/xml' }

breathe_default_project = 'roc'
breathe_domain_by_extension = {'h': 'c'}

# -- Options for HTML output ----------------------------------------------

html_title = '%s %s' % (project, release)

html_theme = 'nature'

html_logo = '../../images/logo80.png'

html_sidebars = {
   '**': ['globaltoc.html', 'searchbox.html'],
}

html_context = {
    'css_files': ['_static/roc.css'],
    'script_files': ['/analytics.js'],
}

html_static_path = ['_static']

# -- Options for manual page output ---------------------------------------

man_pages = [
    ('manuals/roc_send', 'roc-send', u'send real-time audio', [], 1),
    ('manuals/roc_recv', 'roc-recv', u'receive real-time audio', [], 1),
    ('manuals/roc_conv', 'roc-conv', u'convert audio', [], 1),
]
