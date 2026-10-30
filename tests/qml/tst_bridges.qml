import QtQuick
import QtTest
import wynpix.bridges
import wynpix.tests

// Theme and Actions as published by QmlTestSetup over the
// Fixture singleton (bridgetestfixture.h). The input items are siblings of
// the TestCase: a TestCase is not visible, so its own children would not
// receive pointer events.
Item {
    id: root

    width: 200
    height: 200

    // A binding consumer, to check that notifications reach bindings.
    QtObject {
        id: consumer

        property color background: Theme.tokens.surfaceCanvas
    }

    Item {
        id: keySink

        anchors.fill: parent
        focus: true
        Keys.onPressed: event => event.accepted = Actions.handleKeyEvent(event)

        MouseArea {
            id: wheelArea

            anchors.fill: parent
            onWheel: wheel => wheel.accepted = Actions.handleWheelEvent(wheel)
        }
    }

    TestCase {
        id: testCase

        readonly property int wheelNotch: 120
        readonly property string settingsGlyph: String.fromCodePoint(0xF6A9)

        name: "Bridges"
        when: windowShown

        SignalSpy {
            id: tokensSpy
            target: Theme
            signalName: "tokensChanged"
        }
        SignalSpy {
            id: fontsSpy
            target: Theme
            signalName: "fontsChanged"
        }
        SignalSpy {
            id: iconFontSpy
            target: Theme
            signalName: "iconFontFamilyChanged"
        }

        function init() {
            for (const spy of [tokensSpy, fontsSpy, iconFontSpy])
                spy.clear();
        }

        function test_themeSwitchNotifiesColors() {
            const background = Theme.tokens.surfaceCanvas;
            Fixture.switchTheme();

            compare(tokensSpy.count, 1);
            compare(fontsSpy.count, 0);
            compare(iconFontSpy.count, 0);
            verify(!Qt.colorEqual(Theme.tokens.surfaceCanvas, background));
            verify(Qt.colorEqual(consumer.background, Theme.tokens.surfaceCanvas));
        }

        function test_iconGlyphs() {
            compare(Theme.glyph(FluentIcons.Settings20), testCase.settingsGlyph);
            verify(Theme.iconFontFamily.length > 0);
            verify(Theme.compactIconSize > 0);
            verify(Theme.standardIconSize > Theme.compactIconSize);
        }

        function test_invokeAndShortcuts() {
            verify(Actions.invoke("nextImage"));
            compare(Fixture.lastInvoked, "nextImage");
            verify(!Actions.invoke("noSuchAction"));
            compare(Actions.shortcutFor("nextImage"), "Right");
            compare(Actions.shortcutFor("noSuchAction"), "");
        }

        function test_keyForwarding() {
            keySink.forceActiveFocus();
            keyClick(Qt.Key_Right, Qt.ControlModifier);

            compare(Fixture.lastKey, Qt.Key_Right);
            compare(Fixture.lastModifiers, Qt.ControlModifier);
        }

        function test_wheelForwarding() {
            mouseWheel(wheelArea, wheelArea.width / 2, wheelArea.height / 2, 0, -testCase.wheelNotch);

            compare(Fixture.lastWheelAngleDelta.y, -testCase.wheelNotch);
        }
    }
}
