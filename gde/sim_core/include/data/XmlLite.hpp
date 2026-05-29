#include "base/gateway/Coptional.hpp"
#include "base/gateway/Cstring.hpp"
#include "base/gateway/Cvector.hpp"
#pragma once


namespace growth {
namespace xml_lite {

/// Read entire file; empty string on failure.
String read_file(const String &path);

/// Extract attribute value from a tag fragment, e.g. interaction_id="harvest_plant".
Optional<String> attr(const String &tag, const String &name);

/// All lines containing a substring (e.g. "<interaction_def").
Vector<String> lines_containing(const String &text, const String &needle);

/// Text content of simple elements: <file>defs/x.xml</file>
Vector<String> element_texts(const String &text, const String &element_name);

/// Pack refs from data manifest: pack_id + path attributes on <pack> tags.
struct PackRef {
	String pack_id;
	String path;
};

Vector<PackRef> parse_pack_refs(const String &manifest_xml);

} // namespace xml_lite
} // namespace growth
