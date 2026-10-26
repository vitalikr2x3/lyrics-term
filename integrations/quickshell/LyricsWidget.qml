// Skeleton: shows the current lyric line written by `lyrics-term --export`.
// Not tested yet. Check the Quickshell API names (Quickshell.env, Quickshell.Io,
// StdioCollector) against the docs for your installed version before using it.

import QtQuick
import Quickshell
import Quickshell.Io

Item {
    id: root

    property string exportPath: Quickshell.env("HOME") + "/.cache/lyrics-term/current.txt"
    property string line: ""

    // Re-reads the file on a timer. The writer replaces the file atomically,
    // so the read never sees a partial line.
    Process {
        id: reader
        command: ["cat", root.exportPath]
        stdout: StdioCollector {
            onStreamFinished: root.line = this.text.trim()
        }
    }

    Timer {
        interval: 300
        running: true
        repeat: true
        onTriggered: reader.running = true
    }

    Text {
        anchors.centerIn: parent
        text: root.line
        color: "#00ffcc"
        font.family: "Noto Sans"
        font.pixelSize: 14
    }
}
