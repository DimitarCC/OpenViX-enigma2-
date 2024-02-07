from Plugins.Plugin import PluginDescriptor
from Plugins.Extensions.IPTV.M3UProvider import M3UProvider
from Plugins.Extensions.IPTV.IPTVProviders import providers


def sessionstart(reason, **kwargs):
	# Example providers
	# TiViBG provider
	tivibg = M3UProvider()
	tivibg.iptv_service_provider = "TiViBG"
	tivibg.url = ""
	tivibg.offset = 1
	tivibg.refresh_interval = 2
	tivibg.search_criteria = "tvg-id=\"tivi.{SID}\""
	tivibg.scheme = "tivi"
	providers[tivibg.scheme] = tivibg
	
	# Gainedge provider
	gainedge = M3UProvider()
	gainedge.iptv_service_provider = "Gainedge"
	gainedge.url = ""
	gainedge.offset = 1
	gainedge.refresh_interval = -1
	gainedge.search_criteria = "tvg-id=\"{SID}\""
	gainedge.scheme = "gainedge"
	providers[gainedge.scheme] = gainedge

def Plugins(path, **kwargs):
	try:
		return [PluginDescriptor(where=PluginDescriptor.WHERE_AUTOSTART, fnc=sessionstart, needsRestart=False)]
	except ImportError:
		return PluginDescriptor()

