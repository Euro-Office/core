/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
#include "workbook_facts.h"

#include "../../DesktopEditor/common/File.h"
#include "../../DesktopEditor/xml/include/xmlutils.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

namespace
{
	typedef XmlUtils::CXmlNode Node;

	std::string Utf8(const std::wstring& s)
	{
		return U_TO_UTF8(s);
	}

	// Element name without its namespace prefix.
	std::wstring LocalName(Node& node)
	{
		std::wstring name = node.GetName();
		std::wstring::size_type colon = name.find(L':');
		return std::wstring::npos == colon ? name : name.substr(colon + 1);
	}

	std::vector<Node> Children(Node& node, const std::wstring& localName)
	{
		std::vector<Node> all, result;
		node.GetChilds(all);
		for (Node& child : all)
			if (LocalName(child) == localName)
				result.push_back(child);
		return result;
	}

	bool Child(Node& node, const std::wstring& localName, Node& result)
	{
		std::vector<Node> found = Children(node, localName);
		if (found.empty())
			return false;
		result = found[0];
		return true;
	}

	// All descendants with this local name, in document order.
	void Descendants(Node& node, const std::wstring& localName, std::vector<Node>& result)
	{
		std::vector<Node> all;
		node.GetChilds(all);
		for (Node& child : all)
		{
			if (LocalName(child) == localName)
				result.push_back(child);
			Descendants(child, localName, result);
		}
	}

	// Concatenated text of all <t> descendants (rich text runs).
	std::wstring Text(Node& node)
	{
		std::vector<Node> runs;
		Descendants(node, L"t", runs);
		std::wstring text;
		for (Node& run : runs)
			text += run.GetText();
		return text;
	}

	// "a=1 b=2" of all attributes without a namespace prefix, sorted.
	std::string Attributes(Node& node)
	{
		std::vector<std::wstring> names, values;
		node.GetAllAttributes(names, values);
		std::map<std::wstring, std::wstring> sorted;
		for (size_t i = 0; i < names.size(); ++i)
			if (names[i].find(L':') == std::wstring::npos && names[i] != L"xmlns")
				sorted[names[i]] = values[i];
		std::string result;
		for (const auto& attr : sorted)
			result += (result.empty() ? "" : " ") + Utf8(attr.first) + "=" + Utf8(attr.second);
		return result;
	}

	// The same number written as "8", "8.0" or "8.0000000000000001".
	std::string Number(const std::wstring& value)
	{
		std::string text = Utf8(value);
		char* end = nullptr;
		double number = strtod(text.c_str(), &end);
		if (text.empty() || *end != '\0')
			return text;
		char buffer[64];
		snprintf(buffer, sizeof(buffer), "%.15g", number);
		return buffer;
	}

	std::string Bool(Node& node, const std::wstring& name)
	{
		Node child;
		if (!Child(node, name, child))
			return "0";
		std::wstring val = child.GetAttribute(L"val", L"1");
		return (val == L"0" || val == L"false") ? "0" : "1";
	}

	// "xl/worksheets/../x.xml" -> "xl/x.xml"
	std::wstring Normalize(const std::wstring& path)
	{
		std::vector<std::wstring> parts;
		std::wstring::size_type start = 0;
		while (start <= path.size())
		{
			std::wstring::size_type slash = path.find(L'/', start);
			std::wstring part = path.substr(start, std::wstring::npos == slash ? std::wstring::npos : slash - start);
			if (part == L"..")
			{
				if (!parts.empty())
					parts.pop_back();
			}
			else if (!part.empty() && part != L".")
				parts.push_back(part);
			if (std::wstring::npos == slash)
				break;
			start = slash + 1;
		}
		std::wstring result;
		for (const std::wstring& part : parts)
			result += (result.empty() ? L"" : L"/") + part;
		return result;
	}

	struct Relationship
	{
		std::wstring type;   // last segment of the type URI: "worksheet", "table", ...
		std::wstring target; // package path, e.g. "xl/worksheets/sheet1.xml"
	};

	class Package
	{
	public:
		explicit Package(const std::wstring& dir) : m_dir(dir) {}

		bool Load(const std::wstring& part, Node& root) const
		{
			return root.FromXmlFile(m_dir + L"/" + part);
		}

		// Relationships of a part by id; external targets are kept as they are.
		std::map<std::wstring, Relationship> Rels(const std::wstring& part) const
		{
			std::map<std::wstring, Relationship> result;
			std::wstring::size_type slash = part.rfind(L'/');
			std::wstring dir = std::wstring::npos == slash ? L"" : part.substr(0, slash);
			std::wstring name = std::wstring::npos == slash ? part : part.substr(slash + 1);

			Node root;
			if (!Load((dir.empty() ? L"" : dir + L"/") + L"_rels/" + name + L".rels", root))
				return result;
			std::vector<Node> rels = Children(root, L"Relationship");
			for (Node& rel : rels)
			{
				Relationship r;
				std::wstring type = rel.GetAttribute(L"Type");
				r.type = type.substr(type.rfind(L'/') + 1);
				std::wstring target = rel.GetAttribute(L"Target");
				if (rel.GetAttribute(L"TargetMode") == L"External")
					r.target = target;
				else if (!target.empty() && target[0] == L'/')
					r.target = Normalize(target.substr(1));
				else
					r.target = Normalize(dir + L"/" + target);
				result[rel.GetAttribute(L"Id")] = r;
			}
			return result;
		}

		std::vector<std::wstring> Targets(const std::wstring& part, const std::wstring& type) const
		{
			std::vector<std::wstring> result;
			for (const auto& rel : Rels(part))
				if (rel.second.type == type)
					result.push_back(rel.second.target);
			return result;
		}

	private:
		std::wstring m_dir;
	};

	// Effective formatting of each cellXfs entry, without its indexes.
	std::vector<std::string> LoadCellStyles(const Package& package, Node& styles)
	{
		std::map<std::wstring, std::wstring> numFmts;
		Node node;
		if (Child(styles, L"numFmts", node))
			for (Node& fmt : Children(node, L"numFmt"))
				numFmts[fmt.GetAttribute(L"numFmtId")] = fmt.GetAttribute(L"formatCode");

		std::vector<std::string> fonts, fills, borders, result;
		if (Child(styles, L"fonts", node))
			for (Node& font : Children(node, L"font"))
			{
				Node sub;
				std::string f = "b=" + Bool(font, L"b") + " i=" + Bool(font, L"i") + " strike=" + Bool(font, L"strike");
				f += " u=" + std::string(Child(font, L"u", sub) ? Utf8(sub.GetAttribute(L"val", L"single")) : "none");
				f += " sz=" + std::string(Child(font, L"sz", sub) ? Number(sub.GetAttribute(L"val")) : "");
				f += " name=" + std::string(Child(font, L"name", sub) ? Utf8(sub.GetAttribute(L"val")) : "");
				f += " color=(" + std::string(Child(font, L"color", sub) ? Attributes(sub) : "") + ")";
				fonts.push_back(f);
			}
		if (Child(styles, L"fills", node))
			for (Node& fill : Children(node, L"fill"))
			{
				Node pattern, color;
				std::string f;
				if (Child(fill, L"patternFill", pattern))
				{
					f = "pattern=" + Utf8(pattern.GetAttribute(L"patternType", L"none"));
					f += " fg=(" + std::string(Child(pattern, L"fgColor", color) ? Attributes(color) : "") + ")";
					f += " bg=(" + std::string(Child(pattern, L"bgColor", color) ? Attributes(color) : "") + ")";
				}
				else
					f = "gradient";
				fills.push_back(f);
			}
		if (Child(styles, L"borders", node))
			for (Node& border : Children(node, L"border"))
			{
				std::string b;
				for (const wchar_t* side : {L"left", L"right", L"top", L"bottom", L"diagonal"})
				{
					Node edge, color;
					std::string style = Child(border, side, edge) ? Utf8(edge.GetAttribute(L"style")) : "";
					b += Utf8(side) + "=" + style;
					if (!style.empty() && Child(edge, L"color", color))
						b += "(" + Attributes(color) + ")";
					b += " ";
				}
				borders.push_back(b);
			}
		if (Child(styles, L"cellXfs", node))
			for (Node& xf : Children(node, L"xf"))
			{
				std::wstring id = xf.GetAttribute(L"numFmtId", L"0");
				auto custom = numFmts.find(id);
				std::string s = "numFmt=" + (custom != numFmts.end() ? Utf8(custom->second) : "builtin " + Utf8(id));
				size_t font = (size_t)xf.GetAttributeInt(L"fontId", 0);
				size_t fill = (size_t)xf.GetAttributeInt(L"fillId", 0);
				size_t border = (size_t)xf.GetAttributeInt(L"borderId", 0);
				s += " | font " + (font < fonts.size() ? fonts[font] : "?");
				s += " | fill " + (fill < fills.size() ? fills[fill] : "?");
				s += " | border " + (border < borders.size() ? borders[border] : "?");
				Node alignment;
				s += " | alignment " + (Child(xf, L"alignment", alignment) ? Attributes(alignment) : "");
				result.push_back(s);
			}
		return result;
	}

	void AddWorksheet(const Package& package, const std::wstring& part, const std::string& sheet,
					  const std::vector<std::wstring>& sharedStrings, const std::vector<std::string>& cellStyles,
					  WorkbookFacts& facts)
	{
		Node root;
		if (!package.Load(part, root))
		{
			facts["workbook/" + sheet + "/error"] = "cannot read " + Utf8(part);
			return;
		}

		std::vector<Node> cells;
		Descendants(root, L"c", cells);
		for (Node& c : cells)
		{
			std::string ref = Utf8(c.GetAttribute(L"r"));
			std::wstring type = c.GetAttribute(L"t", L"n");
			Node v, f;
			bool hasValue = Child(c, L"v", v), hasFormula = Child(c, L"f", f);
			std::string value;
			if (type == L"s" && hasValue)
			{
				size_t index = (size_t)atoi(Utf8(v.GetText()).c_str());
				value = index < sharedStrings.size() ? Utf8(sharedStrings[index]) : "?";
				type = L"str";
			}
			else if (type == L"inlineStr")
			{
				value = Utf8(Text(c));
				type = L"str";
			}
			else if (hasValue)
				value = type == L"n" ? Number(v.GetText()) : Utf8(v.GetText());

			if (hasValue || hasFormula || type == L"inlineStr")
			{
				std::string formula = hasFormula ? Utf8(f.GetText()) + "|" + Utf8(f.GetAttribute(L"t")) + " " + Utf8(f.GetAttribute(L"ref")) : "|";
				facts["cells/" + sheet + "/" + ref] = Utf8(type) + "|" + value + "|" + formula;
			}

			size_t style = (size_t)c.GetAttributeInt(L"s", 0);
			if (style != 0)
				facts["styles/" + sheet + "/" + ref] = style < cellStyles.size() ? cellStyles[style] : "?";
		}

		std::vector<Node> merges;
		Descendants(root, L"mergeCell", merges);
		for (Node& merge : merges)
			facts["merges/" + sheet + "/" + Utf8(merge.GetAttribute(L"ref"))] = "";

		for (const auto& rel : package.Rels(part))
		{
			const std::wstring& type = rel.second.type;
			Node r;
			if (type == L"comments" && package.Load(rel.second.target, r))
			{
				std::vector<Node> authors, comments;
				Descendants(r, L"author", authors);
				Descendants(r, L"comment", comments);
				for (Node& comment : comments)
				{
					size_t author = (size_t)comment.GetAttributeInt(L"authorId", 0);
					facts["comments/" + sheet + "/" + Utf8(comment.GetAttribute(L"ref"))] =
						(author < authors.size() ? Utf8(authors[author].GetText()) : "?") + ": " + Utf8(Text(comment));
				}
			}
			else if (type == L"table" && package.Load(rel.second.target, r))
			{
				std::string key = "tables/" + sheet + "/" + Utf8(r.GetAttribute(L"name"));
				facts[key] = "displayName=" + Utf8(r.GetAttribute(L"displayName")) + " ref=" + Utf8(r.GetAttribute(L"ref"));
				std::vector<Node> columns;
				Descendants(r, L"tableColumn", columns);
				std::string names;
				for (Node& column : columns)
					names += Utf8(column.GetAttribute(L"name")) + ";";
				facts[key + "/columns"] = names;
				Node info;
				if (Child(r, L"tableStyleInfo", info))
					facts[key + "/style"] = Attributes(info);
			}
			else if (type == L"pivotTable" && package.Load(rel.second.target, r))
			{
				std::string key = "pivots/" + sheet + "/" + Utf8(r.GetAttribute(L"name"));
				Node location;
				facts[key + "/location"] = Child(r, L"location", location) ? Attributes(location) : "";

				std::vector<Node> dataFields;
				Descendants(r, L"dataField", dataFields);
				std::string data;
				for (Node& field : dataFields)
					data += Utf8(field.GetAttribute(L"name")) + " fld=" + Utf8(field.GetAttribute(L"fld")) +
							" " + Utf8(field.GetAttribute(L"subtotal", L"sum")) + ";";
				facts[key + "/dataFields"] = data;

				for (const std::wstring& cachePart : package.Targets(rel.second.target, L"pivotCacheDefinition"))
				{
					Node cache;
					if (!package.Load(cachePart, cache))
						continue;
					std::vector<Node> fields, sources;
					Descendants(cache, L"cacheField", fields);
					std::string names;
					for (Node& field : fields)
						names += Utf8(field.GetAttribute(L"name")) + ";";
					facts[key + "/cacheFields"] = names;
					Descendants(cache, L"worksheetSource", sources);
					facts[key + "/source"] = sources.empty() ? "" : Attributes(sources[0]);

					for (const std::wstring& recordsPart : package.Targets(cachePart, L"pivotCacheRecords"))
					{
						Node records;
						if (!package.Load(recordsPart, records))
							continue;
						std::string rows;
						for (Node& row : Children(records, L"r"))
						{
							std::vector<Node> items;
							row.GetChilds(items);
							for (Node& item : items)
								rows += Utf8(LocalName(item)) + ":" + Utf8(item.GetAttribute(L"v")) + " ";
							rows += ";";
						}
						facts[key + "/records"] = rows;
					}
				}
			}
		}
	}
}

WorkbookFacts LoadWorkbookFacts(const std::wstring& unpackedDir)
{
	WorkbookFacts facts;
	Package package(unpackedDir);

	std::wstring workbookPart = L"xl/workbook.xml";
	for (const std::wstring& target : package.Targets(L"", L"officeDocument"))
		workbookPart = target;

	Node workbook;
	if (!package.Load(workbookPart, workbook))
	{
		facts["workbook/error"] = "cannot read " + Utf8(workbookPart);
		return facts;
	}
	std::map<std::wstring, Relationship> workbookRels = package.Rels(workbookPart);

	std::vector<std::wstring> sharedStrings;
	for (const auto& rel : workbookRels)
	{
		Node sst;
		if (rel.second.type == L"sharedStrings" && package.Load(rel.second.target, sst))
			for (Node& si : Children(sst, L"si"))
				sharedStrings.push_back(Text(si));
	}

	std::vector<std::string> cellStyles;
	for (const auto& rel : workbookRels)
	{
		Node styles;
		if (rel.second.type == L"styles" && package.Load(rel.second.target, styles))
			cellStyles = LoadCellStyles(package, styles);
	}

	std::vector<Node> sheets;
	Descendants(workbook, L"sheet", sheets);
	std::vector<std::string> sheetNames;
	for (size_t i = 0; i < sheets.size(); ++i)
	{
		Node& sheet = sheets[i];
		std::string name = Utf8(sheet.GetAttribute(L"name"));
		sheetNames.push_back(name);

		std::wstring rid;
		std::vector<std::wstring> names, values;
		sheet.GetAllAttributes(names, values);
		for (size_t a = 0; a < names.size(); ++a)
			if (names[a].size() > 3 && names[a].substr(names[a].size() - 3) == L":id")
				rid = values[a];
		auto rel = workbookRels.find(rid);
		std::string type = rel != workbookRels.end() ? Utf8(rel->second.type) : "?";

		char index[16];
		snprintf(index, sizeof(index), "%02u", (unsigned)i);
		facts["workbook/sheet " + std::string(index)] = name + " type=" + type + " state=" + Utf8(sheet.GetAttribute(L"state", L"visible"));

		if (rel == workbookRels.end())
			continue;
		if (rel->second.type == L"worksheet")
			AddWorksheet(package, rel->second.target, name, sharedStrings, cellStyles, facts);
		else if (rel->second.type == L"chartsheet")
		{
			std::string related;
			for (const auto& sub : package.Rels(rel->second.target))
				related += Utf8(sub.second.type) + ";";
			facts["chartsheets/" + name] = related;
		}
	}

	std::vector<Node> definedNames;
	Descendants(workbook, L"definedName", definedNames);
	for (Node& definedName : definedNames)
	{
		std::wstring local = definedName.GetAttribute(L"localSheetId");
		size_t sheet = local.empty() ? sheetNames.size() : (size_t)atoi(Utf8(local).c_str());
		std::string scope = sheet < sheetNames.size() ? sheetNames[sheet] : "workbook";
		facts["names/" + scope + "/" + Utf8(definedName.GetAttribute(L"name"))] = Utf8(definedName.GetText());
	}

	for (const auto& rel : workbookRels)
	{
		Node cache;
		if (rel.second.type == L"slicerCache" && package.Load(rel.second.target, cache))
			facts["slicers/cache " + Utf8(cache.GetAttribute(L"name"))] = "sourceName=" + Utf8(cache.GetAttribute(L"sourceName"));
	}
	// Slicers hang off the sheets; collect them from every worksheet.
	for (const auto& rel : workbookRels)
	{
		if (rel.second.type != L"worksheet")
			continue;
		for (const std::wstring& slicerPart : package.Targets(rel.second.target, L"slicer"))
		{
			Node slicers;
			if (!package.Load(slicerPart, slicers))
				continue;
			for (Node& slicer : Children(slicers, L"slicer"))
				facts["slicers/" + Utf8(slicer.GetAttribute(L"name"))] =
					"caption=" + Utf8(slicer.GetAttribute(L"caption")) + " cache=" + Utf8(slicer.GetAttribute(L"cache"));
		}
	}

	return facts;
}
