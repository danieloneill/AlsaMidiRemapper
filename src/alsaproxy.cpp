#include "alsaproxy.h"

#include <QVariantMap>
#include <QDebug>
#include <QFile>

AlsaProxy::AlsaProxy(QObject *parent)
    : QObject{parent}
{
    snd_seq_open(&m_seqHandle, "default", SND_SEQ_OPEN_DUPLEX, 0);
    snd_seq_set_client_name(m_seqHandle, "AlsaMidiRemapper");

    m_client = snd_seq_client_id(m_seqHandle);

    m_inport = snd_seq_create_simple_port(m_seqHandle, "amidimap IN",
                                        SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
                                        SND_SEQ_PORT_TYPE_APPLICATION);

    m_outport = snd_seq_create_simple_port(m_seqHandle, "amidimap OUT",
                                         SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
                                         SND_SEQ_PORT_TYPE_APPLICATION);

    printf("IN = %d:%d, OUT = %d:%d\n",
           m_client, m_inport,
           m_client, m_outport);

    int nfds = snd_seq_poll_descriptors_count(m_seqHandle, POLLIN);
    m_pollfds.resize(nfds);
    snd_seq_poll_descriptors(m_seqHandle, m_pollfds.data(), nfds, POLLIN);

    for (auto &pfd : m_pollfds) {
        auto notifier = new QSocketNotifier(pfd.fd, QSocketNotifier::Read, this);
        connect(notifier, &QSocketNotifier::activated, this, &AlsaProxy::handleInput);
        m_notifiers.append(notifier);
    }

    snd_seq_nonblock(m_seqHandle, 1);
}

AlsaProxy::~AlsaProxy()
{
    snd_seq_close(m_seqHandle);
}

void AlsaProxy::emitEvent(const QVariantMap &event)
{
    snd_seq_event_t *ev = event["data"].value<snd_seq_event_t *>();
    ev->data.note.channel = event["channel"].toUInt();
    ev->data.note.note = event["note"].toUInt();
    ev->data.note.velocity = event["velocity"].toUInt();
    ev->data.note.duration = event["duration"].toUInt();

    snd_seq_ev_set_source(ev, m_outport);
    int i = snd_seq_event_output_direct(m_seqHandle, ev);
    if (i < 0)
        qWarning() << "snd_seq_event_output_direct() failed:" << snd_strerror(i);
}

bool AlsaProxy::writeFile(const QString &path, const QByteArray &data)
{
    QFile f(path);
    if( !f.open(QIODevice::WriteOnly) )
        return false;

    f.write(data);
    f.close();

    return true;
}

QByteArray AlsaProxy::readFile(const QString &path)
{
    QFile f(path);
    if( !f.open(QIODevice::ReadOnly) )
        return QByteArray();

    QByteArray data = f.readAll();
    f.close();
    return data;
}

QVariant AlsaProxy::listPorts(int client)
{
    QVariantList ports;

    snd_seq_port_info_t *pinfo;
    snd_seq_port_info_alloca(&pinfo);
    snd_seq_port_info_set_client(pinfo, client); // from above
    snd_seq_port_info_set_port(pinfo, -1);

    while (snd_seq_query_next_port(m_seqHandle, pinfo) >= 0) {
        int port = snd_seq_port_info_get_port(pinfo);
        const char *name = snd_seq_port_info_get_name(pinfo);
        unsigned int caps = snd_seq_port_info_get_capability(pinfo);
        //printf("  Port %d: %s (caps: 0x%x)\n", port, name, caps);

        QVariantMap entry;
        entry["name"] = QString::fromLatin1(name);
        entry["caps"] = caps;
        entry["writable"] = (caps & SND_SEQ_PORT_CAP_SUBS_WRITE) ? true : false;
        entry["readable"] = (caps & SND_SEQ_PORT_CAP_SUBS_READ) ? true : false;
        entry["port"] = port;
        ports.push_back(entry);
    }

    //snd_seq_port_info_free(pinfo);
    return ports;
}

QVariant AlsaProxy::listClients()
{
    QVariantList clients;

    snd_seq_client_info_t *cinfo;
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_client_info_set_client(cinfo, -1);

    while (snd_seq_query_next_client(m_seqHandle, cinfo) >= 0) {
        int client = snd_seq_client_info_get_client(cinfo);
        const char *name = snd_seq_client_info_get_name(cinfo);
        //printf("Client %d: %s\n", client, name);

        QVariantMap entry;
        entry["name"] = QString::fromLatin1(name);
        entry["client"] = client;
        entry["ports"] = listPorts(client);
        clients.push_back(entry);
    }

    //snd_seq_client_info_free(cinfo);
    return clients;
}

bool AlsaProxy::connectPorts(int sourceClient, int sourcePort, int destClient, int destPort)
{
    snd_seq_addr_t src_addr, dest_addr;
    src_addr.client = sourceClient;
    src_addr.port = sourcePort;
    dest_addr.client = destClient;
    dest_addr.port = destPort;

    snd_seq_port_subscribe_t *sub;
    snd_seq_port_subscribe_alloca(&sub);
    snd_seq_port_subscribe_set_sender(sub, &src_addr);
    snd_seq_port_subscribe_set_dest(sub, &dest_addr);
    int r = snd_seq_subscribe_port(m_seqHandle, sub);
    //snd_seq_port_subscribe_free(sub);
    return 0 == r;
}

bool AlsaProxy::disconnectPorts(int sourceClient, int sourcePort, int destClient, int destPort)
{
    snd_seq_addr_t src_addr, dest_addr;
    src_addr.client = sourceClient;
    src_addr.port = sourcePort;
    dest_addr.client = destClient;
    dest_addr.port = destPort;

    snd_seq_port_subscribe_t *sub;
    snd_seq_port_subscribe_alloca(&sub);
    snd_seq_port_subscribe_set_sender(sub, &src_addr);
    snd_seq_port_subscribe_set_dest(sub, &dest_addr);
    int r = snd_seq_unsubscribe_port(m_seqHandle, sub);
    //snd_seq_port_subscribe_free(sub);
    return 0 == r;
}

void AlsaProxy::handleInput() {
    snd_seq_event_t *ev = nullptr;
    while (snd_seq_event_input(m_seqHandle, &ev) >= 0) {
        snd_seq_ev_set_subs(ev);
        snd_seq_ev_set_direct(ev);

        QVariantMap event;
        switch( ev->type ) {
        case SND_SEQ_EVENT_SYSTEM:
            event["type"] = "SND_SEQ_EVENT_SYSTEM";
            break;
        case SND_SEQ_EVENT_NOTE:
            event["type"] = "SND_SEQ_EVENT_NOTE";
            break;
        case SND_SEQ_EVENT_NOTEON:
            event["type"] = "SND_SEQ_EVENT_NOTEON";
            break;
        case SND_SEQ_EVENT_NOTEOFF:
            event["type"] = "SND_SEQ_EVENT_NOTEOFF";
            break;
        case SND_SEQ_EVENT_KEYPRESS:
            event["type"] = "SND_SEQ_EVENT_KEYPRESS";
            break;
        }

        event["channel"] = ev->data.note.channel;
        event["note"] = ev->data.note.note;
        event["duration"] = ev->data.note.duration;
        event["velocity"] = ev->data.note.velocity;
        event["data"] = QVariant::fromValue<snd_seq_event_t *>(ev);

        //qDebug() << event;

        if( m_map.contains(ev->data.note.note) )
        {
            ev->data.note.note = m_map[ev->data.note.note];
            snd_seq_ev_set_source(ev, m_outport);
            snd_seq_event_output_direct(m_seqHandle, ev);
        }
        emit eventReceived(event);
    }
}

void AlsaProxy::setMap(const QVariantMap &map)
{
    QMap< quint8, quint8 > newmap;
    for( const QString &key : map.keys() )
    {
        newmap[ key.toUInt() ] = map[key].toUInt();
    }
    m_map = newmap;
}
