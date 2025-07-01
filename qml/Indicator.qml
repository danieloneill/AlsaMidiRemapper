import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Rectangle {
    id: indicator
    height: 24
    width: 24
    radius: 24
    property color colourActive: Material.accentColor
    property color colourInactive: Qt.alpha( Material.iconColor, 0.5 );
    property color colourDisabled: Material.backgroundDimColor

    color: enabled ? colourInactive : colourDisabled

    function trigger() {
        if( enabled )
            fadeAnim.restart();
    }

    SequentialAnimation {
        id: fadeAnim
        PropertyAction { target: indicator; property: 'color'; value: indicator.colourActive }
        PauseAnimation { duration: 120 }
        PropertyAnimation { target: indicator; property: 'color'; to: indicator.colourInactive; duration: 500 }
    }
}
