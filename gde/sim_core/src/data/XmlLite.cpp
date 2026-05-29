#include "data/XmlLite.hpp"
#include "base/gateway/Cfstream.hpp"
#include "base/gateway/Cios.hpp"
#include "base/gateway/Coptional.hpp"
#include "base/gateway/Csstream.hpp"
#include "base/gateway/Cutility.hpp"

namespace growth {
namespace xml_lite {

namespace {

	String normalise_slashes(String s) {
		for (char &c : s) {
			if (c == '\\') c = '/';
		}
		return s;
	}

} // namespace

String read_file(const String &path) {
	Ifstream in = Cfstream::open_input(normalise_slashes(path), Cios::binary);
	if (!in) return {};
	OStringStream ss;
	ss << in.rdbuf();
	return ss.str();
}

Optional<String> attr(const String &tag, const String &name) {
	const String key = name + "=\"";
	const auto pos          = tag.find(key);
	if (pos == String::npos) return nullopt;
	const auto start = pos + key.size();
	const auto end   = tag.find('"', start);
	if (end == String::npos) return nullopt;
	return tag.substr(start, end - start);
}

Vector<String> lines_containing(const String &text, const String &needle) {
	Vector<String> out;
	IStringStream stream(text);
	String line;
	while (getline(stream, line)) {
		if (line.find(needle) != String::npos)
			out.push_back(line);
	}
	return out;
}

Vector<String> element_texts(const String &text, const String &element_name) {
	Vector<String> out;
	const String open  = "<" + element_name + ">";
	const String close = "</" + element_name + ">";
	size_t pos = 0;
	while (pos < text.size()) {
		const auto o = text.find(open, pos);
		if (o == String::npos) break;
		const auto c = text.find(close, o + open.size());
		if (c == String::npos) break;
		out.push_back(text.substr(o + open.size(), c - (o + open.size())));
		pos = c + close.size();
	}
	return out;
}

Vector<PackRef> parse_pack_refs(const String &manifest_xml) {
	Vector<PackRef> out;
	for (const auto &line : lines_containing(manifest_xml, "<pack")) {
		if (line.find("/>") == String::npos && line.find("</pack>") != String::npos)
			continue;
		PackRef ref;
		if (auto id = attr(line, "pack_id")) ref.pack_id = *id;
		if (auto path = attr(line, "path")) ref.path = *path;
		if (!ref.pack_id.empty() && !ref.path.empty())
			out.push_back(Cutility::move(ref));
	}
	return out;
}

} // namespace xml_lite
} // namespace growth
