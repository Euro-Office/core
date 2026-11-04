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
 * the core checkout) are used. Exit code is non-zero on the first failure.
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

	let failures = 0;
	for (const fixture of fixtures) {
		const ext = path.extname(fixture).toLowerCase();
		const inputName = path.basename(fixture);
		const base = path.basename(fixture, ext);
		const key = ext.substring(1);
		try {
			const bin = wasmConvert(module, fixture, inputName, '.bin', FORMAT[key], CANVAS[key]);
			fs.writeFileSync(path.join(resultsDir, `${base}.bin`), bin);
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
				console.log(`ok   ${base}.bin -> ${outExt} (${out.length} bytes)`);
			}
		} catch (e) {
			failures++;
			console.error(`FAIL ${inputName}: ${e.message}`);
		}
	}
	console.log(`${fixtures.length - failures}/${fixtures.length} fixtures converted`);
	process.exit(failures ? 1 : 0);
}

main().catch((e) => {
	console.error(e);
	process.exit(1);
});
