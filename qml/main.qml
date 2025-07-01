import QtQuick
import QtCore
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Qt.labs.qmlmodels
import QtQuick.Dialogs

Window {
    id: mainWindow
    width: 350
    height: 1200
    visible: true
    title: qsTr("Alsa Midi Remapper")

    Material.theme: Material.Dark

    Settings {
        id: settings
        property string sourceName
        property string destName

        property string data
    }

    Rectangle {
        anchors.fill: parent
        color: Material.backgroundColor
    }

    GridLayout {
        id: clientsGrid
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            margins: 10
        }

        columns: 3
        columnSpacing: 5

        Label { text: qsTr('From:') }
        ComboBox {
            id: sourceCombo
            Layout.fillWidth: true
            textRole: 'label'
            enabled: !buttonLinked.checked
            onActivated: {
                let sourceEntry = sourceCombo.model[ sourceCombo.currentIndex ];
                settings.sourceName = sourceEntry['name'];
                console.log(`Source updated to ${sourceEntry['name']}`);
            }
        }
        Button {
            text: qsTr('⟳ Reload')
            enabled: !buttonLinked.checked
            onClicked: mainWindow.reloadClients();
        }

        Label { text: qsTr('To:') }
        ComboBox {
            id: destCombo
            Layout.fillWidth: true
            textRole: 'label'
            enabled: !buttonLinked.checked
            onActivated: {
                let destEntry = destCombo.model[ destCombo.currentIndex ];
                settings.destName = destEntry['name'];
                console.log(`Dest updated to ${destEntry['name']}`);
            }
        }
        CheckBox {
            id: buttonLinked
            text: qsTr('Connect')
            onClicked: {
                mainWindow.changedConnectionState();
            }
        }
    }

    function changedConnectionState()
    {
        if( buttonLinked.checked )
        {
            let sourceEntry = sourceCombo.model[ sourceCombo.currentIndex ];
            let resultA = AlsaProxy.connectPorts( sourceEntry['client'], sourceEntry['port'], AlsaProxy.clientId(), AlsaProxy.inPortId() );

            let destEntry = destCombo.model[ destCombo.currentIndex ];
            let resultB = AlsaProxy.connectPorts( AlsaProxy.clientId(), AlsaProxy.outPortId(), destEntry['client'], destEntry['port'] );

            return resultA && resultB;
        }

        let sourceEntry = sourceCombo.model[ sourceCombo.currentIndex ];
        let resultA = AlsaProxy.disconnectPorts( sourceEntry['client'], sourceEntry['port'], AlsaProxy.clientId(), AlsaProxy.inPortId() );

        let destEntry = destCombo.model[ destCombo.currentIndex ];
        let resultB = AlsaProxy.disconnectPorts( AlsaProxy.clientId(), AlsaProxy.outPortId(), destEntry['client'], destEntry['port'] );

        return resultA && resultB;
    }

    Component.onCompleted: {
        reloadClients();

        // Remember our previous source/dest selections
        //console.log(`Setting source to ${settings.sourceName}`);
        for( let a=0; a < sourceCombo.model.length; a++ )
        {
            let sourceEntry = sourceCombo.model[ a ];
            if( sourceEntry['name'] === settings.sourceName ) {
                sourceCombo.currentIndex = a;
                break;
            }
        }

        //console.log(`Setting dest to ${settings.destName}`);
        for( let b=0; b < destCombo.model.length; b++ )
        {
            let destEntry = destCombo.model[ b ];
            if( destEntry['name'] === settings.destName ) {
                destCombo.currentIndex = b;
                break;
            }
        }

        mainWindow.loadSettings();
        mainWindow.commitMap();
    }

    function reloadClients() {
        const clients = AlsaProxy.listClients();
        //console.log(JSON.stringify(clients,null,2));

        let sources = [];
        let dests = [];

        for( let client of clients )
        {
            if( client['name'] === 'AlsaMidiRemapper' )
                continue;

            for( let port of client['ports'] )
            {
                if( port['readable'] )
                {
                    let ent = { 'label':`${client['name']} - ${client['client']}:${port['port']}`, 'name':client['name'], 'client':client['client'], 'port':port['port'] };
                    sources.push(ent);
                }

                if( port['writable'] )
                {
                    let ent = { 'label':`${client['name']} - ${client['client']}:${port['port']}`, 'name':client['name'], 'client':client['client'], 'port':port['port'] };
                    dests.push(ent);
                }
            }
        }

        sourceCombo.model = sources;
        destCombo.model = dests;
    }

    RowLayout {
        id: bar
        anchors {
            left: parent.left
            right: parent.right
            top: clientsGrid.bottom
            margins: 10
        }


        Button {
            text: qsTr('Clear')
            Layout.fillWidth: true
            onClicked: {
                noteModel.clear();
                mainWindow.notemap = {};
                mainWindow.saveSettings();
            }
        }
        Button {
            text: qsTr("Save")
            Layout.fillWidth: true
            onClicked: saveDialogue.open();
        }
        Button {
            text: qsTr("Load")
            Layout.fillWidth: true
            onClicked: loadDialogue.open();
        }
    }

    ListView {
        id: table
        anchors {
            top: bar.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            margins: 10
        }

        clip: true
        delegate: RowLayout {
            id: rowDelegate
            width: table.width
            required property var modelData
            required property int index
            function trigger() { indicator.trigger(); }
            Indicator { id: indicator; enabled: rowEnabled.checked }
            Switch {
                id: rowEnabled
                checked: rowDelegate.modelData['enabled']
                onClicked: {
                    noteModel.setProperty(rowDelegate.index, "enabled", checked);
                    mainWindow.saveSettings();
                }
            }
            Label { text: rowDelegate.modelData['sourceNote']; enabled: rowEnabled.checked }
            Label { text: '→'; enabled: rowEnabled.checked }
            TextField {
                text: rowDelegate.modelData['mappedNote']
                enabled: rowEnabled.checked
                onTextEdited: {
                    noteModel.setProperty(rowDelegate.index, "mappedNote", parseInt(text));
                    mainWindow.saveSettings();
                }
                validator: IntValidator {}
            }
            //Button { text: qsTr('⟳'); enabled: rowEnabled.checked }
        }

        model: noteModel
    }

    ListModel {
        id: noteModel
    }

    function loadSettings()
    {
        load(settings.data);
    }

    function loadFile(path)
    {
        load(AlsaProxy.readFile(path));
        saveSettings();
    }

    function load(buf)
    {
        try {
            const dataObj = JSON.parse(buf);

            noteModel.clear();
            mainWindow.notemap = {};
            for( let entry of dataObj['entries'] ) {
                mainWindow.notemap[entry['source']] = noteModel.count;
                const noteObj = { 'enabled':entry['enabled'], 'activity':false, 'sourceNote':entry['source'], 'mappedNote':entry['mapped'] };
                noteModel.append(noteObj);
            }
        } catch(e) {
            console.error("Error: " + e);
        }
    }

    function saveSettings()
    {
        settings.data = save();
        //console.log(`Settings.data: ${settings.data}`);
        commitMap();
    }

    function commitMap() {
        let newmap = {};
        for( let a=0; a < noteModel.count; a++ )
        {
            const noteObj = noteModel.get(a);
            newmap[ noteObj['sourceNote'] ] = noteObj['mappedNote'];
        }
        AlsaProxy.setMap(newmap);
    }

    function saveFile(path)
    {
        AlsaProxy.writeFile(path, save());
    }

    function save()
    {
        let entries = [];
        for( let a=0; a < noteModel.count; a++ )
        {
            const noteObj = noteModel.get(a);
            entries.push( { 'enabled':noteObj['enabled'], 'source':noteObj['sourceNote'], 'mapped':noteObj['mappedNote'] } );
        }
        let dataObj = { 'entries':entries };
        return JSON.stringify( dataObj );
    }

    function triggerNote(noteIdx)
    {
        let obj = table.itemAtIndex(noteIdx);
        if( !obj )
            return;
        obj.trigger();
    }

    property var notemap: false
    Connections {
        target: AlsaProxy
        function onEventReceived(event) {
            if( !mainWindow.notemap )
                mainWindow.notemap = {};

            if( event.type !== "SND_SEQ_EVENT_NOTEON" && event.type !== "SND_SEQ_EVENT_NOTEOFF" )
                return;

            //console.log(`Event: t=${event.type} c=${event.channel} n=${event.note} d=${event.duration} v=${event.velocity}`);
            if( !mainWindow.notemap.hasOwnProperty(event.note) ) {
                mainWindow.notemap[event.note] = noteModel.count;
                const noteObj = { 'enabled':true, 'activity':(event.type === "SND_SEQ_EVENT_NOTEON"), 'sourceNote':event.note, 'mappedNote':event.note };
                noteModel.append(noteObj);
                mainWindow.saveSettings();
            } else {
                const noteIdx = mainWindow.notemap[event.note];
                const noteObj = noteModel.get(noteIdx);
                if( !noteObj['enabled'] )
                    return;
                noteModel.setProperty(noteIdx, 'activity', (event.type === "SND_SEQ_EVENT_NOTEON"));
                event.note = noteObj['mappedNote'];

                if( event.type === 'SND_SEQ_EVENT_NOTEON' )
                    mainWindow.triggerNote(noteIdx);
            }

            //AlsaProxy.emitEvent(event);
        }
    }

    FileDialog {
        id: saveDialogue
        currentFolder: StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
        defaultSuffix: 'json'
        fileMode: FileDialog.SaveFile
        nameFilters: ["JSON files (*.json)"]
        onAccepted: {
            let rawPath = (selectedFile+'').substring(7);
            mainWindow.saveFile(rawPath);
        }
    }

    FileDialog {
        id: loadDialogue
        currentFolder: StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
        fileMode: FileDialog.OpenFile
        nameFilters: ["JSON files (*.json)"]
        onAccepted: {
            let rawPath = (selectedFile+'').substring(7);
            mainWindow.loadFile(rawPath);
        }
    }
}
