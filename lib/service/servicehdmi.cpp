#include <lib/base/ebase.h>
#include <lib/base/eerror.h>
#include <lib/base/init_num.h>
#include <lib/base/init.h>
#include <lib/base/modelinformation.h>
#include <lib/base/nconfig.h>
#include <lib/base/object.h>
#include <lib/driver/avswitch.h>
#include <lib/dvb/decoder.h>
#include <lib/dvb/encoder.h>
#include <lib/service/servicehdmi.h>
#include <lib/service/service.h>

#include <string>

eServiceFactoryHDMI::eServiceFactoryHDMI()
{
	ePtr<eServiceCenter> sc;

	eServiceCenter::getPrivInstance(sc);
	if (sc)
	{
		std::list<std::string> extensions;
		sc->addServiceFactory(eServiceFactoryHDMI::id, this, extensions);
	}

	m_service_info = new eStaticServiceHDMIInfo();
}

eServiceFactoryHDMI::~eServiceFactoryHDMI()
{
	ePtr<eServiceCenter> sc;

	eServiceCenter::getPrivInstance(sc);
	if (sc)
	{
		sc->removeServiceFactory(eServiceFactoryHDMI::id);
	}
}

DEFINE_REF(eServiceFactoryHDMI)

RESULT eServiceFactoryHDMI::play(const eServiceReference &ref, ePtr<iPlayableService> &ptr)
{
	ptr = new eServiceHDMI(ref);
	return 0;
}

RESULT eServiceFactoryHDMI::record(const eServiceReference &ref, ePtr<iRecordableService> &ptr)
{
	ptr = new eServiceHDMIRecord(ref);
	return 0;
}

RESULT eServiceFactoryHDMI::list(const eServiceReference &, ePtr<iListableService> &ptr)
{
	ptr = nullptr;
	return -1;
}

RESULT eServiceFactoryHDMI::info(const eServiceReference &ref, ePtr<iStaticServiceInformation> &ptr)
{
	ptr = m_service_info;
	return 0;
}

RESULT eServiceFactoryHDMI::offlineOperations(const eServiceReference &, ePtr<iServiceOfflineOperations> &ptr)
{
	ptr = nullptr;
	return -1;
}

DEFINE_REF(eStaticServiceHDMIInfo)

eStaticServiceHDMIInfo::eStaticServiceHDMIInfo()
{
}

RESULT eStaticServiceHDMIInfo::getName(const eServiceReference &ref, std::string &name)
{
	if (ref.name.length())
	{
		name = ref.name;
	}
	else
	{
		name = "HDMI IN";
	}
	return 0;
}

int eStaticServiceHDMIInfo::getLength(const eServiceReference &ref)
{
	return -1;
}

int eStaticServiceHDMIInfo::getInfo(const eServiceReference &ref, int w)
{
	return iServiceInformation::resNA;
}

long long eStaticServiceHDMIInfo::getFileSize(const eServiceReference &ref)
{
	return 0;
}

eServiceHDMI::eServiceHDMI(eServiceReference ref)
 : m_ref(ref), m_decoder_index(0), m_noaudio(false)
{
	eModelInformation &modelinformation = eModelInformation::getInstance();
	m_b_hdmiin_fhd = modelinformation.getValue("hdmifhdin") == "True";
}

eServiceHDMI::~eServiceHDMI()
{
}

DEFINE_REF(eServiceHDMI);

RESULT eServiceHDMI::connectEvent(const sigc::slot<void(iPlayableService*,int)> &event, ePtr<eConnection> &connection)
{
	connection = new eConnection((iPlayableService*)this, m_event.connect(event));
	return 0;
}

RESULT eServiceHDMI::start()
{
	eAVSwitch::getInstance()->startStopHDMIIn(true, !m_noaudio, 1);
#ifndef HAVE_HDMIIN_DM
	m_decoder = new eTSMPEGDecoder(NULL, m_decoder_index);
	m_decoder->setVideoPID(1, 0);
	if (!m_noaudio)
		m_decoder->setAudioPID(1, 0);
	m_decoder->play();
#endif
	m_event(this, evStart);
	m_event((iPlayableService*)this, evVideoSizeChanged);
	m_event((iPlayableService*)this, evVideoGammaChanged);
	return 0;
}

RESULT eServiceHDMI::stop()
{
	eAVSwitch::getInstance()->startStopHDMIIn(false, true, 1);
#ifndef HAVE_HDMIIN_DM
	m_decoder = NULL;
#endif
	m_event(this, evStopped);
	return 0;
}

RESULT eServiceHDMI::setTarget(int target, bool noaudio = false)
{
	m_decoder_index = target;
	m_noaudio = noaudio;
	return 0;
}

RESULT eServiceHDMI::info(ePtr<iServiceInformation> &i)
{
	i = this;
	return 0;
}

RESULT eServiceHDMI::getName(std::string &name)
{
	if (m_ref.name.length())
	{
		name = m_ref.name;
	}
	else
	{
		name = "HDMI IN";
	}
	return 0;
}

int eServiceHDMI::getInfo(int w)
{
	switch (w)
	{
		case sVideoHeight: return m_b_hdmiin_fhd ? 1080 : 720;
		case sVideoWidth: return m_b_hdmiin_fhd ? 1920 : 1280;
		case sFrameRate: return 50;
		case sProgressive: return 1;
		case sGamma: return 0;
		case sAspect: return 1;
	}

	return resNA;
}

std::string eServiceHDMI::getInfoString(int w)
{
	switch (w)
	{
	case sVideoInfo:
	{
		char buff[100];
		snprintf(buff, sizeof(buff), "%d|%d|50|1|0|1",
				m_b_hdmiin_fhd ? 1080 : 720,
				m_b_hdmiin_fhd ? 1920 : 1280
				);
		std::string videoInfo = buff;
		return videoInfo;
	}
	case sServiceref:
		return m_ref.toString();
	default:
		break;
	}
	return iServiceInformation::getInfoString(w);
}

ePtr<iServiceInfoContainer> eServiceHDMI::getInfoObject(int w)
{
	return NULL;
}

DEFINE_REF(eServiceHDMIRecord);

eServiceHDMIRecord::eServiceHDMIRecord(const eServiceReference &ref)
{
	m_ref = ref;
	m_state = stateIdle;
	m_target_fd = -1;
	m_error = 0;
	m_encoder_fd = -1;
	m_buffersize = -1;
	m_thread = NULL;
}

RESULT eServiceHDMIRecord::prepare(const char *filename, time_t begTime, time_t endTime, int eit_event_id, const char *name, const char *descr, const char *tags, bool descramble, bool recordecm, int packetsize)
{
	m_filename = filename;

	if (m_state == stateIdle)
	{
		return doPrepare();
	}
	return -1;
}

RESULT eServiceHDMIRecord::prepareStreaming(bool descramble, bool includeecm)
{
	return -1;
}

RESULT eServiceHDMIRecord::start(bool simulate)
{
	m_simulate = simulate;
	m_event((iRecordableService*)this, evStart);
	return doRecord();
}

RESULT eServiceHDMIRecord::stop()
{
	if (!m_simulate)
		eDebug("[eServiceHDMIRecord][stop] stop recording!");
	if (m_state == stateRecording)
	{
		if (m_thread)
		{
			m_thread->stop();
			m_thread->stopSaveMetaInformation();
		}
		if (m_target_fd >= 0)
		{
			::close(m_target_fd);
			m_target_fd = -1;
		}

		m_state = statePrepared;
	} else if (!m_simulate)
		eDebug("[eServiceHDMIRecord][stop] (was not recording)");
	if (m_state == statePrepared)
	{
		m_thread = NULL;
		if (eEncoder::getInstance()) eEncoder::getInstance()->freeEncoder(m_encoder_fd);
		m_encoder_fd = -1;
		m_state = stateIdle;
	}
	m_event((iRecordableService*)this, evRecordStopped);
	return 0;
}

int eServiceHDMIRecord::doPrepare()
{
	eDebug("[eServiceHDMIRecord][doPrepare] Record to %s , encoder_fd is %d", m_filename.c_str(), m_encoder_fd);
	if (m_encoder_fd < 0)
	{
		if (eEncoder::getInstance())
		{
			m_encoder_fd = eEncoder::getInstance()->allocateHDMIEncoder(m_ref.toString(), m_buffersize);
		}
		eDebug("[eServiceHDMIRecord][doPrepare] Hdmi encoder %s , encoder_fd is %d", m_filename.c_str(), m_encoder_fd);
		if (m_encoder_fd < 0)
		{
			eDebug("[eServiceHDMIRecord][doPrepare] Hdmi encoder %s , encoder_fd is %d", m_filename.c_str(), m_encoder_fd);		
			return -1;
		}
	}
	m_state = statePrepared;
	return 0;
}

int eServiceHDMIRecord::doRecord()
{
	eDebug("[eServiceHDMIRecord][doRecord] entered Recording to %s...", m_filename.c_str());
	int err = doPrepare();
	if (err)
	{
		m_error = errTuneFailed;
		m_event((iRecordableService*)this, evRecordFailed);
		return err;
	}

	if (!m_thread && !m_simulate)
	{
		eDebug("[eServiceHDMIRecord][doRecord] Recording to %s...", m_filename.c_str());
		::remove(m_filename.c_str());
		int fd = ::open(m_filename.c_str(), O_WRONLY | O_CREAT | O_LARGEFILE | O_CLOEXEC, 0666);
		if (fd < 0)
		{
			eDebug("[eServiceHDMIRecord][doRecord] can't open recording file: %m");
			m_error = errOpenRecordFile;
			m_event((iRecordableService*)this, evRecordFailed);
			return errOpenRecordFile;
		}

		m_thread = new eDVBRecordFileThread(188, 20, m_buffersize);
		m_thread->setTargetFD(fd);

		m_target_fd = fd;
	}

	eDebug("[eServiceHDMIRecord] start recording...");

	if (m_state != stateRecording)
	{
		if (m_thread && m_encoder_fd >= 0)
		{
			m_thread->startSaveMetaInformation(m_filename);
			m_thread->start(m_encoder_fd);
		}
		m_state = stateRecording;
	}

	m_error = 0;
	m_event((iRecordableService*)this, evRecordRunning);
	return 0;
}

RESULT eServiceHDMIRecord::stream(ePtr<iStreamableService> &ptr)
{
	ptr = NULL;
	return -1;
}

RESULT eServiceHDMIRecord::subServices(ePtr<iSubserviceList> &ptr)
{
	ptr = NULL;
	return -1;
}

RESULT eServiceHDMIRecord::frontendInfo(ePtr<iFrontendInformation> &ptr)
{
	ptr = this;
	return 0;
}

RESULT eServiceHDMIRecord::connectEvent(const sigc::slot<void(iRecordableService*,int)> &event, ePtr<eConnection> &connection)
{
	connection = new eConnection((iRecordableService*)this, m_event.connect(event));
	return 0;
}

eAutoInitPtr<eServiceFactoryHDMI> init_eServiceFactoryHDMI(eAutoInitNumbers::service + 1, "eServiceFactoryHDMI");
