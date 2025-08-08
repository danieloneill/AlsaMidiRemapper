#ifndef ALSAPROXY_H
#define ALSAPROXY_H

#include <QDebug>
#include <QObject>
#include <QMap>
#include <QSocketNotifier>
#include <QVariant>

#include <alsa/asoundlib.h>
#include <sys/poll.h>

class MapEntry : public QObject
{
	Q_OBJECT

	public:
		MapEntry(QObject *parent = nullptr)
			: QObject(parent),
			m_enabled{true},
			m_note_in{0},
			m_note_out{0},
			m_latency{0}
		{
		}
		MapEntry(const MapEntry &in)
			: QObject()
		{
			m_enabled = in.m_enabled;
			m_note_in = in.m_note_in;
			m_note_out = in.m_note_out;
			m_latency = in.m_latency;
		}
		MapEntry &operator=(const MapEntry &in)
		{
			if( this != &in )
			{
				m_enabled = in.m_enabled;
				m_note_in = in.m_note_in;
				m_note_out = in.m_note_out;
				m_latency = in.m_latency;
			}
			return *this;
		}

		bool m_enabled;
		quint16	m_note_in;
		quint16	m_note_out;
		quint16	m_latency;
};
Q_DECLARE_METATYPE(MapEntry)
QDebug operator<<(QDebug debug, const MapEntry &c);

class AlsaProxy : public QObject
{
    Q_OBJECT

    int m_client, m_inport, m_outport;

public:
    explicit AlsaProxy(QObject *parent = nullptr);
    ~AlsaProxy();

    Q_INVOKABLE void emitEvent(const QVariantMap &event);

    Q_INVOKABLE bool writeFile(const QString &path, const QByteArray &data);
    Q_INVOKABLE QByteArray readFile(const QString &path);

    Q_INVOKABLE QVariant listClients();
    Q_INVOKABLE QVariant listPorts(int client);
    Q_INVOKABLE bool connectPorts(int sourceClient, int sourcePort, int destClient, int destPort);
    Q_INVOKABLE bool disconnectPorts(int sourceClient, int sourcePort, int destClient, int destPort);

    Q_INVOKABLE int clientId() { return m_client; }
    Q_INVOKABLE int inPortId() { return m_inport; }
    Q_INVOKABLE int outPortId() { return m_outport; }

    Q_INVOKABLE void setMap(const QVariantMap &map);

private:
    snd_seq_t *m_seqHandle = nullptr;
    QVector<pollfd> m_pollfds;
    QList<QSocketNotifier *> m_notifiers;
    QMap< quint8, MapEntry > m_map;
    QMap< quint8, bool > m_xhit_sentinel;
    QMap< quint8, quint16 > m_latency;

private slots:
    void handleInput();

signals:
    void eventReceived(QVariant event);
};

#endif // ALSAPROXY_H
