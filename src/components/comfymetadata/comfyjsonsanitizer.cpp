#include "comfyjsonsanitizer.h"

#include <QByteArrayView>

#include <array>
#include <string_view>

namespace {
constexpr std::string_view kNullLiteral = "null";
constexpr std::array<std::string_view, 3> kNonStandardLiterals = {
    "NaN",
    "Infinity",
    "-Infinity",
};

// Characters that can continue a bare JSON token (number, true, false, null)
// or a Python-style literal. A literal is only recognised when neither of its
// neighbours is one of these, so "NaNx" or "1-Infinity" are left alone.
bool isTokenCharacter(char byte)
{
    return (byte >= '0' && byte <= '9') || (byte >= 'A' && byte <= 'Z') ||
           (byte >= 'a' && byte <= 'z') || byte == '_' || byte == '.' ||
           byte == '+' || byte == '-';
}

// Returns the non-standard literal that starts at position as a complete
// token, or an empty view when there is none.
std::string_view matchLiteralAt(QByteArrayView json, qsizetype position)
{
    const QByteArrayView rest = json.sliced(position);
    for (const std::string_view literal : kNonStandardLiterals) {
        const qsizetype literalSize = static_cast<qsizetype>(literal.size());
        if (!rest.startsWith(QByteArrayView(literal.data(), literalSize)))
            continue;

        const qsizetype end = position + literalSize;
        const bool startsAtBoundary =
            position == 0 || !isTokenCharacter(json[position - 1]);
        const bool endsAtBoundary =
            end == json.size() || !isTokenCharacter(json[end]);
        if (startsAtBoundary && endsAtBoundary)
            return literal;
    }
    return {};
}
} // namespace

QByteArray ComfyJsonSanitizer::replaceNonStandardNumberLiterals(const QByteArray &json)
{
    QByteArray sanitized;
    sanitized.reserve(json.size());

    bool insideString = false;
    bool escaped = false;
    qsizetype position = 0;

    while (position < json.size()) {
        const char byte = json.at(position);

        if (insideString) {
            if (escaped)
                escaped = false;
            else if (byte == '\\')
                escaped = true;
            else if (byte == '"')
                insideString = false;
            sanitized.append(byte);
            ++position;
            continue;
        }

        if (byte == '"') {
            insideString = true;
            sanitized.append(byte);
            ++position;
            continue;
        }

        const std::string_view literal = matchLiteralAt(json, position);
        if (!literal.empty()) {
            sanitized.append(kNullLiteral.data(),
                             static_cast<qsizetype>(kNullLiteral.size()));
            position += static_cast<qsizetype>(literal.size());
            continue;
        }

        sanitized.append(byte);
        ++position;
    }

    return sanitized;
}
