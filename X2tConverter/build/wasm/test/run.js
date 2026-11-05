#!/usr/bin/env node
/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 * Smoke test for x2t.wasm: converts sample documents to the editor's binary
 * format and back, and compares the results with a native x2t when one is
 * given. The harness follows CryptPad's onlyoffice-x2t-wasm test.js.
 *
 *   node test/run.js <dist dir with x2t.js and x2t.wasm> [fixture file or dir ...]
 *   X2T_NATIVE=/path/to/x2t node test/run.js <dist dir>
 *
 * Without fixtures the sample files listed in DEFAULT_FIXTURES (relative to
 * the core checkout) are used. Exit code is non-zero when any fixture fails.
 *
 * x2t can exit with 0 and still write an empty document (a reader that found
 * nothing to read), so every output is checked too: a bin must carry the
 * editor's signature and a minimal size, an OOXML/ODF package must be a zip
 * holding its main part, and different fixtures must not convert to outputs
 * of exactly the same size.
 */

'use strict';

const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { execFileSync } = require('node:child_process');

const CORE_ROOT = path.resolve(__dirname, '..', '..', '..', '..');
const DEFAULT_FIXTURES = [
	'Test/Applications/x2tTester/input_word_proc',
	'Test/Applications/x2tTester/input_sheets',
	'Test/Applications/x2tTester/input_slides',
];
const ROUND_TRIPS = {
	'.docx': ['.docx', '.odt'],
	'.odt': ['.docx', '.odt'],
	'.rtf': ['.docx'],
	'.xlsx': ['.xlsx', '.ods'],
	'.ods': ['.xlsx', '.ods'],
	'.csv': ['.xlsx'],
	'.pptx': ['.pptx', '.odp'],
	'.odp': ['.pptx', '.odp'],
};
const FORMAT = {
	docx: 0x41, odt: 0x43, rtf: 0x44, txt: 0x45,
	pptx: 0x81, odp: 0x83,
	xlsx: 0x101, ods: 0x103, csv: 0x104,
};
const CANVAS = { docx: 0x2001, odt: 0x2001, rtf: 0x2001, txt: 0x2001, xlsx: 0x2002, ods: 0x2002, csv: 0x2002, pptx: 0x2003, odp: 0x2003 };
const WORK = '/working';

// Editor.bin header, see BinReaderWriterDefines.h and Presentation/Converter.cpp
const BIN_SIGNATURE = { 0x2001: 'DOCY;', 0x2002: 'XLSY;', 0x2003: 'PPTY;' };
// A bin always carries styles, theme and settings; an empty document from a
// failed reader is ~1 KB
const MIN_BIN_BYTES = 4096;
const MAIN_PART = {
	'.docx': 'word/document.xml',
	'.xlsx': 'xl/workbook.xml',
	'.pptx': 'ppt/presentation.xml',
	'.odt': 'content.xml',
	'.ods': 'content.xml',
	'.odp': 'content.xml',
};

function params(fileFrom, fileTo, formatFrom, formatTo, fontDir, themeDir) {
	return '<?xml version="1.0" encoding="utf-8"?>'
		+ '<TaskQueueDataConvert xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema">'
		+ `<m_sFileFrom>${fileFrom}</m_sFileFrom>`
		+ `<m_sFileTo>${fileTo}</m_sFileTo>`
		+ `<m_sFontDir>${fontDir}</m_sFontDir>`
		+ `<m_sThemeDir>${themeDir}</m_sThemeDir>`
		+ (formatFrom ? `<m_nFormatFrom>${formatFrom}</m_nFormatFrom>` : '')
		+ (formatTo ? `<m_nFormatTo>${formatTo}</m_nFormatTo>` : '')
		+ '<m_bIsNoBase64>false</m_bIsNoBase64>'
		+ '<m_nCsvTxtEncoding>46</m_nCsvTxtEncoding>'
		+ '<m_nCsvDelimiter>4</m_nCsvDelimiter>'
		+ '</TaskQueueDataConvert>';
}

function removeTree(FS, p) {
	if (!FS.analyzePath(p).exists) {
		return;
	}
	if (FS.isDir(FS.stat(p).mode)) {
		FS.readdir(p).filter((e) => e !== '.' && e !== '..').forEach((e) => removeTree(FS, `${p}/${e}`));
		FS.rmdir(p);
	} else {
		FS.unlink(p);
	}
}

function resetWorkDir(FS) {
	removeTree(FS, WORK);
	for (const dir of [WORK, `${WORK}/fonts`, `${WORK}/themes`, `${WORK}/media`]) {
		FS.mkdir(dir);
	}
}

function wasmConvert(module, inputPath, inputName, outExt, formatFrom, formatTo) {
	const FS = module.FS;
	resetWorkDir(FS);
	FS.writeFile(`${WORK}/${inputName}`, fs.readFileSync(inputPath));
	const fileFrom = `${WORK}/${inputName}`;
	const fileTo = `${WORK}/${inputName}${outExt}`;
	FS.writeFile(`${WORK}/params.xml`, params(fileFrom, fileTo, formatFrom, formatTo, `${WORK}/fonts/`, `${WORK}/themes`));
	const code = module.ccall('main1', 'number', ['string'], [`${WORK}/params.xml`]);
	if (code !== 0) {
		throw new Error(`x2t.wasm exited with ${code} for ${inputName} -> ${outExt}`);
	}
	return Buffer.from(FS.readFile(fileTo, { encoding: 'binary' }));
}

// Central directory of a zip: name -> uncompressed size
function zipEntries(buf) {
	const EOCD = 0x06054b50;
	const CEN = 0x02014b50;
	let eocd = -1;
	for (let i = buf.length - 22; i >= Math.max(0, buf.length - 22 - 0xffff); i--) {
		if (buf.readUInt32LE(i) === EOCD) {
			eocd = i;
			break;
		}
	}
	if (eocd < 0) {
		return null;
	}
	const count = buf.readUInt16LE(eocd + 10);
	let pos = buf.readUInt32LE(eocd + 16);
	const entries = new Map();
	for (let n = 0; n < count; n++) {
		if (pos + 46 > buf.length || buf.readUInt32LE(pos) !== CEN) {
			return null;
		}
		const size = buf.readUInt32LE(pos + 24);
		const nameLen = buf.readUInt16LE(pos + 28);
		const extraLen = buf.readUInt16LE(pos + 30);
		const commentLen = buf.readUInt16LE(pos + 32);
		entries.set(buf.toString('utf8', pos + 46, pos + 46 + nameLen), size);
		pos += 46 + nameLen + extraLen + commentLen;
	}
	return entries;
}

function checkBin(buf, canvas) {
	const signature = BIN_SIGNATURE[canvas];
	if (buf.toString('latin1', 0, signature.length) !== signature) {
		throw new Error(`bin does not start with ${signature}`);
	}
	if (buf.length < MIN_BIN_BYTES) {
		throw new Error(`bin has only ${buf.length} bytes, expected at least ${MIN_BIN_BYTES}`);
	}
}

function checkPackage(buf, outExt) {
	const entries = zipEntries(buf);
	if (!entries) {
		throw new Error(`${outExt} output is not a zip package`);
	}
	const main = MAIN_PART[outExt];
	if (!entries.get(main)) {
		throw new Error(`${outExt} output has no ${main} (entries: ${[...entries.keys()].join(', ') || 'none'})`);
	}
}

// Different inputs converting to byte-identical sizes means the reader
// produced the same (empty) document for all of them
function checkDistinctSizes(results) {
	const byDirection = new Map();
	for (const r of results) {
		if (!byDirection.has(r.direction)) {
			byDirection.set(r.direction, []);
		}
		byDirection.get(r.direction).push(r);
	}
	const suspicious = new Set();
	for (const [direction, list] of byDirection) {
		for (let i = 0; i < list.length; i++) {
			for (let j = i + 1; j < list.length; j++) {
				if (list[i].size === list[j].size && !list[i].input.equals(list[j].input)) {
					console.error(`FAIL ${direction}: ${list[i].fixture} and ${list[j].fixture} both gave ${list[i].size} bytes`);
					suspicious.add(list[i].fixture);
					suspicious.add(list[j].fixture);
				}
			}
		}
	}
	return suspicious;
}

function nativeConvert(x2tBinary, inputPath, outExt, formatFrom, formatTo) {
	const work = fs.mkdtempSync(path.join(os.tmpdir(), 'x2t-native-'));
	try {
		const inputName = path.basename(inputPath);
		const fileFrom = path.join(work, inputName);
		const fileTo = `${fileFrom}${outExt}`;
		fs.copyFileSync(inputPath, fileFrom);
		fs.mkdirSync(path.join(work, 'fonts'));
		fs.mkdirSync(path.join(work, 'themes'));
		const xml = path.join(work, 'params.xml');
		fs.writeFileSync(xml, params(fileFrom, fileTo, formatFrom, formatTo, `${work}/fonts/`, `${work}/themes`));
		execFileSync(x2tBinary, [xml], { stdio: 'pipe' });
		return fs.readFileSync(fileTo);
	} finally {
		fs.rmSync(work, { recursive: true, force: true });
	}
}

function collectFixtures(args) {
	const roots = args.length ? args : DEFAULT_FIXTURES.map((p) => path.join(CORE_ROOT, p));
	const files = [];
	for (const root of roots) {
		if (!fs.existsSync(root)) {
			continue;
		}
		if (fs.statSync(root).isDirectory()) {
			for (const entry of fs.readdirSync(root)) {
				const full = path.join(root, entry);
				if (fs.statSync(full).isFile() && ROUND_TRIPS[path.extname(entry).toLowerCase()]) {
					files.push(full);
				}
			}
		} else if (ROUND_TRIPS[path.extname(root).toLowerCase()]) {
			files.push(root);
		}
	}
	return files;
}

async function main() {
	const [distDir, ...fixtureArgs] = process.argv.slice(2);
	if (!distDir) {
		console.error('usage: node test/run.js <dist dir> [fixture ...]');
		process.exit(2);
	}
	const createX2tModule = require(path.resolve(distDir, 'x2t.js'));
	const module = await createX2tModule({
		locateFile: (file) => path.resolve(distDir, file),
		print: () => {},
		printErr: (text) => console.warn(`[x2t] ${text}`),
	});
	const nativeX2t = process.env.X2T_NATIVE;
	const resultsDir = path.resolve('results');
	fs.mkdirSync(resultsDir, { recursive: true });

	const fixtures = collectFixtures(fixtureArgs);
	if (!fixtures.length) {
		console.error('no fixtures found');
		process.exit(2);
	}

	const failed = new Set();
	const results = [];
	for (const fixture of fixtures) {
		const ext = path.extname(fixture).toLowerCase();
		const inputName = path.basename(fixture);
		const base = path.basename(fixture, ext);
		const key = ext.substring(1);
		try {
			const input = fs.readFileSync(fixture);
			const bin = wasmConvert(module, fixture, inputName, '.bin', FORMAT[key], CANVAS[key]);
			fs.writeFileSync(path.join(resultsDir, `${base}.bin`), bin);
			checkBin(bin, CANVAS[key]);
			results.push({ fixture: inputName, direction: `${ext} -> bin`, size: bin.length, input });
			console.log(`ok   ${inputName} -> bin (${bin.length} bytes)`);

			if (nativeX2t) {
				const nativeBin = nativeConvert(nativeX2t, fixture, '.bin', FORMAT[key], CANVAS[key]);
				const ratio = bin.length / nativeBin.length;
				const status = ratio > 0.9 && ratio < 1.1 ? 'ok  ' : 'WARN';
				console.log(`${status} ${inputName} -> bin: wasm ${bin.length} vs native ${nativeBin.length} bytes`);
			}

			for (const outExt of ROUND_TRIPS[ext]) {
				const binPath = path.join(resultsDir, `${base}.bin`);
				const out = wasmConvert(module, binPath, `${base}.bin`, outExt, CANVAS[key], FORMAT[outExt.substring(1)]);
				fs.writeFileSync(path.join(resultsDir, `${base}${outExt}`), out);
				checkPackage(out, outExt);
				results.push({ fixture: inputName, direction: `${ext} -> bin -> ${outExt}`, size: out.length, input });
				console.log(`ok   ${base}.bin -> ${outExt} (${out.length} bytes)`);
			}
		} catch (e) {
			failed.add(inputName);
			console.error(`FAIL ${inputName}: ${e.message}`);
		}
	}
	for (const fixture of checkDistinctSizes(results)) {
		failed.add(fixture);
	}
	console.log(`${fixtures.length - failed.size}/${fixtures.length} fixtures converted`);
	process.exit(failed.size ? 1 : 0);
}

main().catch((e) => {
	console.error(e);
	process.exit(1);
});
