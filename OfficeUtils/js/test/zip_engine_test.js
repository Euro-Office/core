#!/usr/bin/env node
// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only
//
// Checks the zip engine (AscCommon.CZLibEngineJS) in the built fonts.js/fonts.wasm,
// which the editors use for every zip they read or write in the browser
// (table styles in spreadsheets, forms, custom XML, version history, ...).
//
// Usage: node zip_engine_test.js <dir with fonts.js and fonts.wasm>
//
// The module is built for the web, so it runs here with just enough of a
// browser around it: window/self/document globals and a fetch that reads the
// .wasm from disk.

"use strict";

const assert = require("assert");
const fs = require("fs");
const path = require("path");
const vm = require("vm");
const zlib = require("zlib");
const { pathToFileURL, fileURLToPath } = require("url");

function loadEngine(dir) {
	const script = path.resolve(dir, "fonts.js");
	const scriptUrl = pathToFileURL(script).href;

	return new Promise((resolve, reject) => {
		let loadCount = 0;
		Object.assign(globalThis, {
			window: globalThis,
			self: globalThis,
			document: { currentScript: { src: scriptUrl } },
			location: { href: scriptUrl, protocol: "file:" },
			AscFonts: {
				// Called once when the script ran and once when the wasm is ready.
				// The editors' libfont/engine.js then marks the zip engine ready.
				onLoadModule: () => {
					if (++loadCount !== 2) return;
					const ZLib = globalThis.AscCommon.CZLibEngineJS;
					ZLib.prototype.isModuleInit = true;
					resolve(ZLib);
				},
			},
			fetch: async (url) =>
				new Response(fs.readFileSync(fileURLToPath(url)), {
					headers: { "Content-Type": "application/wasm" },
				}),
		});
		setTimeout(() => reject(new Error("fonts.wasm did not load in 30 s")), 30000).unref();
		vm.runInThisContext(fs.readFileSync(script, "utf8"), { filename: script });
	});
}

// A zip written without the engine: one deflated entry
function makeZip(name, data) {
	const nameBytes = Buffer.from(name);
	const deflated = zlib.deflateRawSync(data);
	const crc = zlib.crc32(data);

	const local = Buffer.alloc(30);
	local.writeUInt32LE(0x04034b50, 0);
	local.writeUInt16LE(20, 4); // version needed
	local.writeUInt16LE(8, 8); // deflate
	local.writeUInt32LE(crc, 14);
	local.writeUInt32LE(deflated.length, 18);
	local.writeUInt32LE(data.length, 22);
	local.writeUInt16LE(nameBytes.length, 26);

	const central = Buffer.alloc(46);
	central.writeUInt32LE(0x02014b50, 0);
	central.writeUInt16LE(20, 4); // version made by
	central.writeUInt16LE(20, 6); // version needed
	central.writeUInt16LE(8, 10); // deflate
	central.writeUInt32LE(crc, 16);
	central.writeUInt32LE(deflated.length, 20);
	central.writeUInt32LE(data.length, 24);
	central.writeUInt16LE(nameBytes.length, 28);
	// local header offset: 0

	const centralOffset = local.length + nameBytes.length + deflated.length;
	const centralSize = central.length + nameBytes.length;
	const end = Buffer.alloc(22);
	end.writeUInt32LE(0x06054b50, 0);
	end.writeUInt16LE(1, 8);
	end.writeUInt16LE(1, 10);
	end.writeUInt32LE(centralSize, 12);
	end.writeUInt32LE(centralOffset, 16);

	return Buffer.concat([local, nameBytes, deflated, central, nameBytes, end]);
}

// Compressible, like the XML parts of an Office file
function sampleXml() {
	let xml = '<?xml version="1.0" encoding="UTF-8"?>\n<tableStyles>';
	for (let i = 0; i < 500; i++) xml += `<tableStyle name="TableStyleMedium${i}" pivot="0" count="7"/>`;
	return Buffer.from(xml + "</tableStyles>");
}

const tests = {
	"reads a deflated entry of a zip it did not write"(ZLib) {
		const data = sampleXml();
		const zip = new ZLib();
		assert.ok(zip.open(makeZip("xl/styles.xml", data)), "open failed");
		assert.deepStrictEqual(zip.getPaths(), ["xl/styles.xml"]);
		const file = zip.getFile("xl/styles.xml");
		assert.ok(file, "getFile returned nothing");
		assert.ok(Buffer.from(file).equals(data), "the entry's content differs from what was zipped");
		zip.close();
	},

	"writes a zip and reads it back"(ZLib) {
		const data = sampleXml();
		const small = Buffer.from("x");
		const zip = new ZLib();
		assert.ok(zip.create(), "create failed");
		zip.addFile("a/big.xml", new Uint8Array(data));
		zip.addFile("small.txt", new Uint8Array(small));
		const saved = zip.save();
		assert.ok(saved, "save returned nothing");
		const archive = Buffer.from(saved); // a view into wasm memory, copy it before close
		zip.close();
		assert.ok(archive.length < data.length, `the archive (${archive.length} bytes) is not compressed`);

		assert.ok(zip.open(archive), "open of the saved archive failed");
		assert.deepStrictEqual(zip.getPaths().sort(), ["a/big.xml", "small.txt"]);
		assert.ok(Buffer.from(zip.getFile("a/big.xml")).equals(data), "a/big.xml differs");
		assert.ok(Buffer.from(zip.getFile("small.txt")).equals(small), "small.txt differs");
		zip.close();
	},
};

async function main() {
	const dir = process.argv[2];
	if (!dir) {
		console.error("usage: node zip_engine_test.js <dir with fonts.js and fonts.wasm>");
		process.exit(2);
	}
	const ZLib = await loadEngine(dir);

	let failed = 0;
	for (const [name, test] of Object.entries(tests)) {
		try {
			test(ZLib);
			console.log(`ok - ${name}`);
		} catch (e) {
			failed++;
			console.log(`not ok - ${name}\n  ${e.message}`);
		}
	}
	process.exit(failed ? 1 : 0);
}

main().catch((e) => {
	console.error(e);
	process.exit(1);
});
