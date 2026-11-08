#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QTest>

#include "components/comfymetadata/comfyjsonsanitizer.h"
#include "testsuites.h"

namespace {
// Shape of the node that broke metadata reading in a real ComfyUI image:
// InpaintCanvas reports NaN from IS_CHANGED, which json.dumps() writes bare.
constexpr const char *kNodeWithNanJson =
    R"json({"562":{"inputs":{},"class_type":"InpaintCanvas","is_changed":[NaN]}})json";
constexpr const char *kNodeId = "562";
constexpr const char *kIsChangedKey = "is_changed";
} // namespace

class ComfyJsonSanitizerTests : public QObject {
    Q_OBJECT

private slots:
    void replacesNonStandardLiterals_data();
    void replacesNonStandardLiterals();
    void leavesStringContentUntouched_data();
    void leavesStringContentUntouched();
    void stringStateSurvivesEscapesBeforeALiteral();
    void onlyCompleteTokensAreReplaced_data();
    void onlyCompleteTokensAreReplaced();
    void validJsonIsReturnedUnchanged();
    void emptyInputStaysEmpty();
    void sanitizedGraphParsesWhereTheRawOneDoesNot();
};

void ComfyJsonSanitizerTests::replacesNonStandardLiterals_data() {
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("nan in array")
        << QByteArray(R"json({"is_changed":[NaN]})json")
        << QByteArray(R"json({"is_changed":[null]})json");
    QTest::newRow("nan as object value")
        << QByteArray(R"json({"value":NaN})json")
        << QByteArray(R"json({"value":null})json");
    QTest::newRow("infinity")
        << QByteArray(R"json([Infinity])json")
        << QByteArray(R"json([null])json");
    QTest::newRow("negative infinity")
        << QByteArray(R"json([-Infinity])json")
        << QByteArray(R"json([null])json");
    QTest::newRow("several literals")
        << QByteArray(R"json([NaN, Infinity,-Infinity,NaN])json")
        << QByteArray(R"json([null, null,null,null])json");
    QTest::newRow("whitespace around literal")
        << QByteArray("{\"a\" :\n NaN \t}")
        << QByteArray("{\"a\" :\n null \t}");
}

void ComfyJsonSanitizerTests::replacesNonStandardLiterals() {
    QFETCH(QByteArray, input);
    QFETCH(QByteArray, expected);

    QCOMPARE(ComfyJsonSanitizer::replaceNonStandardNumberLiterals(input), expected);
}

void ComfyJsonSanitizerTests::leavesStringContentUntouched_data() {
    QTest::addColumn<QByteArray>("input");

    QTest::newRow("literals inside a string")
        << QByteArray(R"json({"text":"NaN Infinity -Infinity"})json");
    QTest::newRow("literal inside a key")
        << QByteArray(R"json({"NaN":1,"Infinity":2})json");
    QTest::newRow("escaped quote does not end the string")
        << QByteArray(R"json({"text":"say \"NaN\" now"})json");
    QTest::newRow("escaped backslash before the closing quote")
        << QByteArray(R"json({"path":"C:\\NaN\\"})json");
}

void ComfyJsonSanitizerTests::leavesStringContentUntouched() {
    QFETCH(QByteArray, input);

    QCOMPARE(ComfyJsonSanitizer::replaceNonStandardNumberLiterals(input), input);
}

void ComfyJsonSanitizerTests::stringStateSurvivesEscapesBeforeALiteral() {
    const QByteArray input =
        R"json({"text":"say \"NaN\" now","path":"C:\\","value":NaN})json";
    const QByteArray expected =
        R"json({"text":"say \"NaN\" now","path":"C:\\","value":null})json";

    QCOMPARE(ComfyJsonSanitizer::replaceNonStandardNumberLiterals(input), expected);
}

void ComfyJsonSanitizerTests::onlyCompleteTokensAreReplaced_data() {
    QTest::addColumn<QByteArray>("input");

    QTest::newRow("suffix") << QByteArray("[NaNx]");
    QTest::newRow("prefix") << QByteArray("[xInfinity]");
    QTest::newRow("digit before") << QByteArray("[1NaN]");
    QTest::newRow("sign glued to a number") << QByteArray("[1-Infinity]");
    QTest::newRow("longer identifier") << QByteArray("[Infinity2]");
}

void ComfyJsonSanitizerTests::onlyCompleteTokensAreReplaced() {
    QFETCH(QByteArray, input);

    QCOMPARE(ComfyJsonSanitizer::replaceNonStandardNumberLiterals(input), input);
}

void ComfyJsonSanitizerTests::validJsonIsReturnedUnchanged() {
    const QByteArray input =
        R"json({"1":{"inputs":{"seed":42,"cfg":1.0,"flag":true,"none":null},"class_type":"KSampler"}})json";

    QCOMPARE(ComfyJsonSanitizer::replaceNonStandardNumberLiterals(input), input);
}

void ComfyJsonSanitizerTests::emptyInputStaysEmpty() {
    QVERIFY(ComfyJsonSanitizer::replaceNonStandardNumberLiterals(QByteArray()).isEmpty());
}

void ComfyJsonSanitizerTests::sanitizedGraphParsesWhereTheRawOneDoesNot() {
    const QByteArray raw(kNodeWithNanJson);

    QJsonParseError rawError{};
    const QJsonDocument rawDocument = QJsonDocument::fromJson(raw, &rawError);
    QVERIFY2(rawError.error != QJsonParseError::NoError,
             "Qt is expected to reject a bare NaN literal");
    QVERIFY(rawDocument.isNull());

    QJsonParseError sanitizedError{};
    const QJsonDocument sanitizedDocument = QJsonDocument::fromJson(
        ComfyJsonSanitizer::replaceNonStandardNumberLiterals(raw), &sanitizedError);
    QCOMPARE(sanitizedError.error, QJsonParseError::NoError);
    QVERIFY(sanitizedDocument.isObject());

    const QJsonObject node = sanitizedDocument.object().value(kNodeId).toObject();
    QCOMPARE(node.value("class_type").toString(), QStringLiteral("InpaintCanvas"));
    const QJsonArray isChanged = node.value(kIsChangedKey).toArray();
    QCOMPARE(isChanged.size(), 1);
    QVERIFY(isChanged.at(0).isNull());
}

int runComfyJsonSanitizerTests(int argc, char **argv) {
    ComfyJsonSanitizerTests tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "tst_comfyjsonsanitizer.moc"
