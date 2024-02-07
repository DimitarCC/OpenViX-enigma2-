#!/usr/bin/python
# -*- coding: utf-8 -*-

#  Software Feed Plugin by Sven H

# language-support
from Components.Language import language
from os import environ as os_environ
import gettext
from Tools.Directories import resolveFilename, SCOPE_LANGUAGE, SCOPE_PLUGINS
lang = language.getLanguage()
os_environ["LANGUAGE"] = lang[:2]
gettext.bindtextdomain("enigma2", resolveFilename(SCOPE_LANGUAGE))
gettext.textdomain("enigma2")
gettext.bindtextdomain("enigma2-plugins", resolveFilename(SCOPE_LANGUAGE))
gettext.bindtextdomain("SvenHPlugins", "%s%s" % (resolveFilename(SCOPE_PLUGINS), "Extensions/SvenHPlugins/locale"))

def _(txt):
	t = gettext.dgettext("SvenHPlugins", txt)
	if t == txt:
		t = gettext.gettext(txt)
	if t == txt:
		t = gettext.dgettext("enigma2-plugins", txt)
	return t
