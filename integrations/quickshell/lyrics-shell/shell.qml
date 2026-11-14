// Lyrics bar for Quickshell: shows the current synced lyric line at the bottom of the screen.
// It starts lyrics-term in headless mode by itself, so no exec-once line is needed.
// Run with: qs -c lyrics

import QtQuick
import Quickshell
import Quickshell.Io

ShellRoot {
    id: root

    // ---- Settings: change these ----
    readonly property string player: ""               // "" = any player; or "spotify", "audacious", "mpv"
    readonly property string accent: "#00ffcc"        // text color
    readonly property string fontFamily: "Noto Sans"
    readonly property int fontSize: 18
    readonly property int barHeight: 44
    readonly property int fadeMs: 180                 // fade time between lines
    readonly property real backgroundOpacity: 0.4     // 0 = fully transparent bar

    readonly property string exportPath: Quickshell.env("HOME") + "/.cache/lyrics-term/current.txt"

    property string line: ""                          // the line we want to show

    // Start lyrics-term in headless mode: it writes the current line to exportPath
    Process {
        id: lyricsProcess
        running: true
        command: [
            "sh", "-c",
            "mkdir -p \"$HOME/.cache/lyrics-term\" && exec lyrics-term --headless --export \"$HOME/.cache/lyrics-term/current.txt\""
                + (root.player !== "" ? " --player " + root.player : "")
        ]
    }

    // Read the export file; it is replaced atomically, so we never see a partial line
    FileView {
        id: exportFile
        path: root.exportPath
        watchChanges: true
        onFileChanged: reload()
        onLoaded: root.setLine(text().trim())
    }

    // Fade out the old line, swap the text, fade in
    function setLine(next) {
        if (next === root.line) return;
        root.line = next;
        lyricText.opacity = 0;
        swapTimer.restart();
    }

    Timer {
        id: swapTimer
        interval: root.fadeMs
        onTriggered: {
            lyricText.text = root.line;
            lyricText.opacity = root.line === "" ? 0 : 1;
        }
    }

    PanelWindow {
        anchors {
            bottom: true
            left: true
            right: true
        }
        implicitHeight: root.barHeight
        color: "transparent"
        exclusiveZone: 0                             // do not push other windows aside

        // Background, only visible when there is something to show
        Rectangle {
            anchors.fill: parent
            color: "#000000"
            opacity: lyricText.text !== "" ? root.backgroundOpacity : 0
            Behavior on opacity { NumberAnimation { duration: root.fadeMs } }
        }

        Text {
            id: lyricText
            anchors.centerIn: parent
            width: parent.width - 40
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            color: root.accent
            font.family: root.fontFamily
            font.pixelSize: root.fontSize
            opacity: 0
            Behavior on opacity { NumberAnimation { duration: root.fadeMs } }
        }
    }
}
