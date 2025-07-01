#ifndef ALSAPROXY_H
#define ALSAPROXY_H

#include <QObject>
#include <QMap>
#include <QSocketNotifier>
#include <QVariant>

#include <alsa/asoundlib.h>
#include <sys/poll.h>

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
    QMap< quint8, quint8 > m_map;

private slots:
    void handleInput();

signals:
    void eventReceived(QVariant event);
};

#endif // ALSAPROXY_H
