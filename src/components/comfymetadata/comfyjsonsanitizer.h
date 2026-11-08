#pragma once

#include <QByteArray>

// ComfyUI serialises its "prompt" graph with Python's json.dumps(), which
// writes non-finite floats as the bare literals NaN, Infinity and -Infinity.
// Custom nodes routinely produce them (for example IS_CHANGED returning NaN
// to disable caching). RFC 8259 forbids these literals, so QJsonDocument
// rejects the whole document and every other value in the graph is lost.
namespace ComfyJsonSanitizer {
// Returns a copy of json in which every NaN, Infinity and -Infinity token
// outside a string literal is replaced with null. The contents of string
// literals are never modified, and input without such tokens is returned
// unchanged. The result is at most one third larger than the input.
QByteArray replaceNonStandardNumberLiterals(const QByteArray &json);
} // namespace ComfyJsonSanitizer
