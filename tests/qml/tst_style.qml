pragma ComponentBehavior: Bound

import QtQuick
import QtTest
import wynpix.bridges
import wynpix.style
import wynpix.tests

// The wynpix.style controls over the application's dark and light themes
// (Fixture builds them from ThemeStore). Each test checks the colours of the
// flat parts of a control in its states against the Theme colours they come
// from, sampled at points away from text and rounded corners, so the checks
// do not depend on font rendering. test_saveSnapshots writes the whole
// gallery of both schemes next to the test executable for visual review.
//
// The gallery is a sibling of the TestCase: a TestCase is not visible, so
// its own children would not be rendered or receive pointer events.
Item {
    id: root

    width: 720
    height: 640

    Rectangle {
        id: gallery

        anchors.fill: parent
        color: Theme.tokens.surfacePopup

        Flow {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 12

            Button {
                id: button
                text: "Button"
                hoverEnabled: true
            }
            Button {
                id: disabledButton
                text: "Disabled"
                enabled: false
            }
            ToolButton {
                id: toolButton
                text: "Tool"
                hoverEnabled: true
            }
            ComboBox {
                id: comboBox
                model: ["First", "Second", "Third"]
                hoverEnabled: true
            }
            Slider {
                id: slider
                width: 200
                from: 0
                to: 1
                value: 0.5
                hoverEnabled: true
            }
            SpinBox {
                id: spinBox
                from: 0
                to: 10
                value: 5
                hoverEnabled: true
            }
            DoubleSpinBox {
                id: doubleSpinBox
                from: 0
                to: 1
                value: 0.5
                stepSize: 0.1
                hoverEnabled: true
            }
            CheckBox {
                id: checkBox
                text: "Check box"
            }
            RadioButton {
                id: radioButton
                text: "Radio button"
            }
            TextField {
                id: textField
                placeholderText: "Text field"
                hoverEnabled: true
            }
            SearchField {
                id: searchField
                placeholderText: "Search field"
                hoverEnabled: true
            }
            Label {
                id: label
                text: "Label"
            }
            ProgressBar {
                id: progressBar
                width: 200
                value: 0.5
            }
            Item {
                width: scrollBar.width
                height: 200

                ScrollBar {
                    id: scrollBar
                    height: parent.height
                    orientation: Qt.Vertical
                    policy: ScrollBar.AlwaysOn
                    size: 0.3
                    position: 0
                    hoverEnabled: true
                }
            }
        }

        Menu {
            id: menu

            x: 300
            y: 320
            popupType: Popup.Item

            Action {
                text: "Copy"
                shortcut: "Ctrl+C"
            }
            MenuSeparator {}
            MenuItem {
                text: "Disabled"
                enabled: false
            }
        }

        ToolTip {
            id: toolTip
            parent: button
            text: "Tooltip"
        }

        Popup {
            id: popup

            x: 40
            y: 400
            width: 160
            height: 80
        }
    }

    TestCase {
        id: testCase

        // One 8-bit level per channel, plus one for blending.
        readonly property real tolerance: 2 / 255

        name: "Style"
        when: windowShown

        // Global data: every test function without its own _data runs once
        // per scheme and starts with useScheme(data.dark).
        function init_data() {
            return [
                {tag: "dark", dark: true},
                {tag: "light", dark: false},
            ];
        }

        function useScheme(dark) {
            if (Theme.dark !== dark)
                Fixture.switchTheme();
            compare(Theme.dark, dark);
        }

        function cleanup() {
            menu.close();
            toolTip.close();
            popup.close();
            comboBox.popup.close();
            mouseMove(gallery, gallery.width - 1, gallery.height - 1);
            gallery.forceActiveFocus();
            useScheme(true);
        }

        // Colour at (x, y) of item, read from a grab of the whole gallery
        // (at the scene origin): grabImage(item) crops the window at the item's
        // position in its parent, not in the scene.
        function pixel(item, x, y) {
            const point = item.mapToItem(gallery, x, y);
            return grabImage(gallery).pixel(Math.floor(point.x), Math.floor(point.y));
        }

        function colorClose(actual, expected) {
            return Math.abs(actual.r - expected.r) <= tolerance
                && Math.abs(actual.g - expected.g) <= tolerance
                && Math.abs(actual.b - expected.b) <= tolerance;
        }

        function verifyColor(actual, expected, what) {
            verify(colorClose(actual, expected), `${what}: ${actual} instead of ${expected}`);
        }

        // Colour of a translucent fill over the gallery surface.
        function over(fill, opacity) {
            return Color.blend(Theme.tokens.surfacePopup, Color.transparent(fill, 1.0), opacity);
        }

        // Colour of a neutral fill token (translucent ink) over the gallery
        // surface.
        function fillOver(fill) {
            return over(Qt.rgba(fill.r, fill.g, fill.b, 1.0), fill.a);
        }

        function test_button(data) {
            useScheme(data.dark);
            const y = button.height / 2;
            verifyColor(pixel(button, 3, y), fillOver(Theme.tokens.fillRest), "normal");
            mouseMove(button, 3, y);
            verify(button.hovered);
            verifyColor(pixel(button, 3, y), fillOver(Theme.tokens.fillHover), "hovered");
            mousePress(button, 3, y);
            verifyColor(pixel(button, 3, y), fillOver(Theme.tokens.fillPressed), "pressed");
            mouseRelease(button, 3, y);
            mouseMove(gallery, gallery.width - 1, gallery.height - 1);
            button.highlighted = true;
            verifyColor(pixel(button, 3, y), Theme.tokens.accent, "primary");
            // A disabled primary button offers no accent to click.
            button.enabled = false;
            verifyColor(pixel(button, 3, y), fillOver(Theme.tokens.fillRest), "disabled primary");
            verifyColor(button.contentItem.color, Theme.tokens.textPrimary, "disabled primary text");
            button.enabled = true;
            button.highlighted = false;
            compare(disabledButton.contentItem.opacity, StyleConstants.disabledOpacity);
            verify(button.height >= Theme.metrics.buttonHeight);
        }

        function test_toolButton(data) {
            useScheme(data.dark);
            const x = StyleConstants.toolButtonHorizontalInset + 2;
            const y = toolButton.height / 2;
            verifyColor(pixel(toolButton, x, y), Theme.tokens.surfacePopup, "normal (transparent)");
            mouseMove(toolButton, x, y);
            verifyColor(pixel(toolButton, x, y), fillOver(Theme.tokens.fillHover), "hovered");
            mouseMove(gallery, gallery.width - 1, gallery.height - 1);
            toolButton.checkable = true;
            toolButton.checked = true;
            verifyColor(pixel(toolButton, x, y), fillOver(Theme.tokens.accentTint), "checked");
            toolButton.checked = false;
            toolButton.checkable = false;
            // The inset keeps the panel colour off the bar edge.
            verifyColor(pixel(toolButton, x, 1), Theme.tokens.surfacePopup, "inset");
        }

        function test_comboBox(data) {
            useScheme(data.dark);
            const y = comboBox.height / 2;
            verify(comboBox.highlightOnHover);
            verifyColor(pixel(comboBox, 3, y), Theme.tokens.fieldRest, "field");
            verifyColor(pixel(comboBox, 0, y), Theme.tokens.strokeField, "border");
            mouseMove(comboBox, 3, y);
            verifyColor(pixel(comboBox, 3, y), Theme.tokens.fieldHover, "hovered");
            // Focus from a click or the code is not keyboard focus: no ring.
            comboBox.forceActiveFocus();
            verifyColor(pixel(comboBox, 0, y), Theme.tokens.strokeField, "mouse focus border");
            gallery.forceActiveFocus();
            comboBox.forceActiveFocus(Qt.TabFocusReason);
            verify(comboBox.visualFocus);
            verifyColor(pixel(comboBox, 0, y), Theme.tokens.accent, "focus border");
            compare(comboBox.indicator.text, Theme.glyph(FluentIcons.ChevronDown12));

            comboBox.popup.open();
            tryCompare(comboBox.popup, "opened", true);
            const background = comboBox.popup.background;
            verifyColor(pixel(background, background.width / 2, 2), Theme.tokens.surfacePopup, "popup");
            verifyColor(pixel(background, background.width / 2, 0), fillOver(Theme.tokens.strokeDefault),
                        "popup border");
            compare(background.radius, Theme.metrics.contextMenuBorderRadius);
        }

        function test_comboBoxScrollingList() {
            const shortModel = comboBox.model;
            comboBox.model = Array.from({ length: 60 }, (_, i) => "Item " + i);
            comboBox.popup.open();
            tryCompare(comboBox.popup, "opened", true);
            const list = comboBox.popup.contentItem;
            verify(list.visibleArea.heightRatio < 1.0, "the list does not scroll");
            // The rows end where the scroll bar begins.
            compare(list.itemAtIndex(0).width, list.width - StyleConstants.scrollBarThickness);
            comboBox.popup.close();
            comboBox.model = shortModel;
        }

        function test_slider(data) {
            useScheme(data.dark);
            const y = slider.height / 2;
            verifyColor(pixel(slider, 20, y), Theme.tokens.accent, "filled groove");
            verifyColor(pixel(slider, slider.width - 20, y), fillOver(Theme.tokens.strokeStrong), "groove");
            const handle = slider.handle;
            const centerX = handle.x + handle.width / 2;
            verifyColor(pixel(slider, centerX, y), Theme.tokens.accent, "handle");
            verifyColor(pixel(slider, handle.x + 1, y), Theme.tokens.surfacePopup, "handle ring");
            mouseMove(slider, centerX, y);
            verifyColor(pixel(slider, centerX, y),
                        Color.lighter(Theme.tokens.accent, StyleConstants.accentLightFactor),
                        "hovered handle");
            mouseMove(gallery, gallery.width - 1, gallery.height - 1);
            slider.enabled = false;
            verifyColor(pixel(slider, 20, y), fillOver(Theme.tokens.strokeStrong), "disabled fill");
            verifyColor(pixel(slider, centerX, y), over(Theme.tokens.textSecondary, StyleConstants.disabledOpacity),
                        "disabled handle");
            slider.enabled = true;
        }

        function test_spinBoxes_data() {
            return [
                {tag: "int, dark", dark: true, spin: spinBox},
                {tag: "int, light", dark: false, spin: spinBox},
                {tag: "double, dark", dark: true, spin: doubleSpinBox},
                {tag: "double, light", dark: false, spin: doubleSpinBox},
            ];
        }

        function test_spinBoxes(data) {
            useScheme(data.dark);
            const spin = data.spin;
            const field = spin.width / 2;
            verifyColor(pixel(spin, field, 3), Theme.tokens.fieldRest, "field");
            verifyColor(pixel(spin, field, 0), Theme.tokens.strokeField, "border");
            mouseMove(spin, field, spin.height / 2);
            verifyColor(pixel(spin, field, 3), Theme.tokens.fieldHover, "hovered");
            compare(spin.up.indicator.icon, FluentIcons.ChevronUp20);
            compare(spin.down.indicator.icon, FluentIcons.ChevronDown20);

            const value = spin.value;
            mouseClick(spin.up.indicator);
            verify(spin.value > value);
            mouseClick(spin.down.indicator);
            compare(spin.value, value);

            spin.value = spin.to;
            verify(!spin.up.indicator.enabled);
            verify(spin.down.indicator.enabled);
            spin.value = value;
        }

        // Indicators are sampled at the top middle: inside the box or circle
        // and clear of its mark, and on the box outline above it; and at the
        // centre, on the dash or dot. The circle outline is curved there and
        // only partly covers the pixel, so it is not sampled.
        function test_selectionIndicators(data) {
            useScheme(data.dark);
            const box = checkBox.indicator;
            const middle = box.width / 2;
            const inside = 2;
            verifyColor(pixel(box, middle, 0), Theme.tokens.iconDefault, "unchecked box outline");
            verifyColor(pixel(box, middle, inside), Theme.tokens.surfacePopup, "unchecked box");
            mouseClick(checkBox);
            verifyColor(pixel(box, middle, inside), Theme.tokens.accent, "checked box");
            checkBox.tristate = true;
            checkBox.checkState = Qt.PartiallyChecked;
            verifyColor(pixel(box, middle, inside), Theme.tokens.accent, "partially checked box");
            verifyColor(pixel(box, middle, box.height / 2), Theme.tokens.accentText, "dash");
            checkBox.checkState = Qt.Unchecked;
            checkBox.tristate = false;

            const circle = radioButton.indicator;
            verifyColor(pixel(circle, middle, inside), Theme.tokens.surfacePopup, "unchecked circle");
            mouseClick(radioButton);
            verifyColor(pixel(circle, middle, inside), Theme.tokens.accent, "checked circle");
            verifyColor(pixel(circle, middle, circle.height / 2), Theme.tokens.accentText, "dot");
            radioButton.checked = false;

            checkBox.enabled = false;
            compare(checkBox.indicator.opacity, StyleConstants.disabledOpacity);
            checkBox.enabled = true;
        }

        function test_textFields(data) {
            useScheme(data.dark);
            const y = textField.height / 2;
            verifyColor(pixel(textField, 3, y), Theme.tokens.fieldRest, "field");
            verifyColor(pixel(textField, 0, y), Theme.tokens.strokeField, "border");
            mouseMove(textField, 3, y);
            verifyColor(pixel(textField, 3, y), Theme.tokens.fieldHover, "hovered");
            textField.forceActiveFocus();
            verifyColor(pixel(textField, 0, y), Theme.tokens.accent, "focus border");
            verifyColor(pixel(textField, 3, y), Theme.tokens.fieldHover, "focused");

            verifyColor(pixel(searchField, 3, searchField.height / 2), Theme.tokens.fieldRest,
                        "search field");
            verify(searchField.searchIndicator.indicator === null);
            verify(searchField.contentItem.children[0].visible);
            compare(searchField.contentItem.children[0].text, searchField.placeholderText);
            verify(!searchField.clearIndicator.indicator.visible);
            searchField.text = "filter";
            verify(searchField.clearIndicator.indicator.visible);
            compare(searchField.clearIndicator.indicator.icon, FluentIcons.Dismiss16);
            searchField.text = "";
        }

        function test_progressBar(data) {
            useScheme(data.dark);
            const y = progressBar.height / 2;
            verifyColor(pixel(progressBar, 3, y), Theme.tokens.accent, "filled part");
            verifyColor(pixel(progressBar, progressBar.width - 3, y), Theme.tokens.surfaceView,
                        "empty part");
            // The percentage is in the accent text colour over the filled
            // part only.
            const filledText = findChild(progressBar.contentItem, "filledPercentText");
            verify(filledText);
            verify(filledText.clip);
            compare(filledText.width, progressBar.contentItem.width * progressBar.visualPosition);
            verifyColor(filledText.children[0].color, Theme.tokens.accentText, "filled text");
        }

        function test_scrollBar(data) {
            useScheme(data.dark);
            const x = scrollBar.width / 2;
            compare(scrollBar.width, StyleConstants.scrollBarThickness);
            verifyColor(pixel(scrollBar, x, 10), Theme.tokens.scrollbar, "handle");
            mouseMove(scrollBar, x, 10);
            verifyColor(pixel(scrollBar, x, 10), Theme.tokens.scrollbarHover, "hovered handle");
            // The handle never gets shorter than the minimum length.
            scrollBar.size = 0.05;
            verifyColor(pixel(scrollBar, x, StyleConstants.scrollBarMinimumHandleLength - 2),
                        Theme.tokens.scrollbarHover, "minimum handle");
            scrollBar.size = 0.3;
        }

        function test_menu(data) {
            useScheme(data.dark);
            verify(menu.separatorsCollapsible);
            menu.open();
            tryCompare(menu, "opened", true);
            verify(menu.width >= Theme.metrics.contextMenuWidth);
            // The background includes the room for the shadow (the negative
            // insets); the frame starts inside it.
            const background = menu.background;
            const frameTop = -menu.topInset;
            verify(frameTop > 0, "no room for the shadow");
            verifyColor(pixel(background, background.width / 2, frameTop + 2), Theme.tokens.surfacePopup,
                        "menu");
            verifyColor(pixel(background, background.width / 2, frameTop), fillOver(Theme.tokens.strokeDefault),
                        "menu border");

            const item = menu.itemAt(0);
            compare(item.height, Theme.metrics.contextMenuItemHeight);
            const shortcut = item.contentItem.shortcutText;
            compare(shortcut, ShortcutText.of("Ctrl+C"));
            verify(shortcut.length > 0);
            menu.currentIndex = 0;
            verifyColor(pixel(item, 3, item.height / 2),
                        fillOver(Theme.tokens.fillHover),
                        "highlighted item");

            const disabledItem = menu.itemAt(2);
            verify(!disabledItem.enabled);
            verifyColor(disabledItem.contentItem.secondaryColor,
                        Color.transparent(Theme.tokens.textSecondary, StyleConstants.secondaryTextOpacity),
                        "secondary text");
        }

        function test_toolTipAndPopup(data) {
            useScheme(data.dark);
            toolTip.open();
            tryCompare(toolTip, "opened", true);
            const tipBackground = toolTip.background;
            verifyColor(pixel(tipBackground, 3, tipBackground.height / 2), Theme.tokens.surfacePopup,
                        "tooltip");
            compare(tipBackground.radius, Theme.metrics.tooltipBorderRadius);
            verifyColor(toolTip.contentItem.color, Theme.tokens.textPrimary, "tooltip text");

            popup.open();
            tryCompare(popup, "opened", true);
            verifyColor(pixel(popup.background, popup.width / 2, popup.height / 2),
                        Theme.tokens.surfacePopup, "popup");
            verifyColor(pixel(popup.background, popup.width / 2, 0), fillOver(Theme.tokens.strokeDefault),
                        "popup border");

            verifyColor(label.color, Theme.tokens.textPrimary, "label");
        }

        // A theme switch restyles live controls through their bindings, with
        // no reload.
        function test_runtimeThemeSwitch_data() {
            return [{tag: "dark to light", dark: true}];
        }

        function test_runtimeThemeSwitch(data) {
            useScheme(data.dark);
            const y = button.height / 2;
            const darkButton = pixel(button, 3, y);
            const darkField = pixel(comboBox, 3, comboBox.height / 2);
            verifyColor(darkButton, fillOver(Theme.tokens.fillRest), "dark button");

            Fixture.switchTheme();
            verify(!Theme.dark);
            const lightButton = pixel(button, 3, y);
            verifyColor(lightButton, fillOver(Theme.tokens.fillRest), "light button");
            verify(!colorClose(lightButton, darkButton));
            verifyColor(pixel(comboBox, 3, comboBox.height / 2), Theme.tokens.fieldRest,
                        "light combo box");
            verify(!colorClose(pixel(comboBox, 3, comboBox.height / 2), darkField));
            verifyColor(pixel(gallery, 1, 1), Theme.tokens.surfacePopup, "light surface");
        }

        // Writes style-dark.png and style-light.png (the gallery with a menu,
        // a tooltip and a popup open) for visual review.
        function test_saveSnapshots(data) {
            useScheme(data.dark);
            menu.open();
            toolTip.open();
            popup.open();
            tryCompare(menu, "opened", true);
            tryCompare(toolTip, "opened", true);
            tryCompare(popup, "opened", true);
            menu.currentIndex = 0;
            grabImage(gallery).save(Fixture.artifactPath(`style-${data.tag}.png`));
        }
    }
}
