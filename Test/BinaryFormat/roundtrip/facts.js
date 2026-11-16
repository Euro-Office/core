// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only
//
// Structural facts about the open document, read through the sdkjs builder API.
// roundtrip.py inlines this file into the .docbuilder scripts, then calls
// rtFacts("word" | "cell" | "slide") after every OpenFile. The facts must not
// depend on layout or fonts: they describe what survived the trip through
// the binary format, not how it renders.
//
// Script rules (DesktopEditor/doctrenderer/docbuilder_p.cpp, RunTextA): a line
// starting with "builder" is a builder command, a line starting with "#" is
// dropped. Neither may start a line here. ES5 only.

function rtHash(s) {
	// FNV-1a, 32 bit: enough to notice that a long text changed.
	var h = 0x811c9dc5;
	for (var i = 0; i < s.length; i++) {
		h ^= s.charCodeAt(i);
		h = Math.imul(h, 0x01000193) >>> 0;
	}
	return ("0000000" + h.toString(16)).slice(-8);
}

function rtText(s) {
	s = String(s);
	return { length: s.length, fnv1a: rtHash(s), head: s.slice(0, 120) };
}

function rtSafe(f) {
	// A missing API or a throwing call becomes a visible fact instead of a dead script.
	try {
		return f();
	} catch (e) {
		return "error: " + e;
	}
}

function rtCount(list) {
	var out = {};
	for (var i = 0; i < list.length; i++)
		out[list[i]] = (out[list[i]] || 0) + 1;
	return out;
}

function rtClassTypes(list) {
	var out = [];
	for (var i = 0; i < list.length; i++)
		out.push(rtSafe(function () { return list[i].GetClassType(); }));
	return out;
}

function rtWordFacts() {
	var doc = Api.GetDocument();
	return {
		elements: rtSafe(function () {
			var types = [];
			for (var i = 0; i < doc.GetElementsCount(); i++)
				types.push(doc.GetElement(i).GetClassType());
			return { count: types.length, types: rtCount(types) };
		}),
		paragraphs: rtSafe(function () { return doc.GetAllParagraphs().length; }),
		tableRows: rtSafe(function () {
			return doc.GetAllTables().map(function (t) { return t.GetRowsCount(); });
		}),
		drawings: rtSafe(function () { return rtCount(rtClassTypes(doc.GetAllDrawingObjects())); }),
		comments: rtSafe(function () {
			return doc.GetAllComments().map(function (c) { return c.GetText(); }).sort();
		}),
		sections: rtSafe(function () { return doc.GetSections().length; }),
		text: rtSafe(function () { return rtText(doc.GetText()); })
	};
}

function rtCellFacts() {
	return {
		sheets: rtSafe(function () {
			return Api.GetSheets().map(function (sheet) {
				var used = sheet.GetUsedRange();
				var values = rtSafe(function () { return used.GetValue2(); });
				var formulas = rtSafe(function () { return used.GetFormula(); });
				return {
					name: sheet.GetName(),
					usedRange: rtSafe(function () { return used.GetAddress(false, false); }),
					values: rtText(JSON.stringify(values)),
					formulas: rtText(JSON.stringify(formulas)),
					comments: rtSafe(function () { return sheet.GetComments().length; }),
					drawings: rtSafe(function () { return rtCount(rtClassTypes(sheet.GetAllDrawings())); })
				};
			});
		})
	};
}

function rtSlideFacts() {
	return {
		slides: rtSafe(function () {
			return Api.GetPresentation().GetAllSlides().map(function (slide) {
				return {
					drawings: rtSafe(function () { return rtCount(rtClassTypes(slide.GetAllDrawings())); }),
					text: rtSafe(function () {
						return rtText(slide.GetAllShapes().map(function (shape) {
							var content = shape.GetDocContent();
							return content ? content.GetText() : "";
						}).join("\n"));
					})
				};
			});
		})
	};
}

function rtFacts(kind) {
	var collect = { word: rtWordFacts, cell: rtCellFacts, slide: rtSlideFacts }[kind];
	return JSON.stringify({ editor: kind, facts: rtSafe(collect) });
}
