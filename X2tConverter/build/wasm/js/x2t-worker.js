/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 * Web Worker that owns one x2t.wasm instance. Driven by converter.js; see
 * README.md next to this file for the protocol. The in-memory layout mirrors
 * what x2t expects on disk (and what CryptPad's driver established):
 *
 *   /working/<input>          the document to convert
 *   /working/params.xml       TaskQueueDataConvert, as the Document Server writes it
 *   /working/fonts/           fonts for layout-dependent conversions
 *   /working/themes/          presentation theme files
 *   /working/media/           images (input for export, output for import)
 *   /working/pdf.bin          the editor's print output, for to === 'pdf'
 */

'use strict';

// Common/OfficeFileFormats.h
var FORMAT = {
	docx: 0x41, doc: 0x42, odt: 0x43, rtf: 0x44, txt: 0x45, docm: 0x4b, dotx: 0x4c,
	pptx: 0x81, ppt: 0x82, odp: 0x83, pptm: 0x85, potx: 0x87,
	xlsx: 0x101, xls: 0x102, ods: 0x103, csv: 0x104, xlsm: 0x105, xltx: 0x106,
	pdf: 0x201,
};
var CANVAS = { word: 0x2001, cell: 0x2002, slide: 0x2003, pdf: 0x2004 };
var FAMILY = {
	docx: 'word', doc: 'word', odt: 'word', rtf: 'word', txt: 'word', docm: 'word', dotx: 'word',
	pptx: 'slide', ppt: 'slide', odp: 'slide', pptm: 'slide', potx: 'slide',
	xlsx: 'cell', xls: 'cell', ods: 'cell', csv: 'cell', xlsm: 'cell', xltx: 'cell',
	pdf: 'pdf',
};
// ODF inputs go through their OOXML sibling when the direct route fails
var ODF_TO_OOXML = { odt: 'docx', ods: 'xlsx', odp: 'pptx' };
var CSV_UTF8 = 46;
var CSV_COMMA = 4;
var WORK = '/working';

var modulePromise = null;
var baseUrl = '';

function error(code, message, extra) {
	var e = { code: code, message: message };
	if (extra) {
		for (var k in extra) {
			e[k] = extra[k];
		}
	}
	return e;
}

function loadModule() {
	if (modulePromise) {
		return modulePromise;
	}
	modulePromise = new Promise(function(resolve, reject) {
		try {
			importScripts(baseUrl + 'x2t.js');
		} catch (e) {
			reject(error('load', 'could not load x2t.js: ' + e.message));
			return;
		}
		if (typeof createX2tModule !== 'function') {
			reject(error('load', 'x2t.js did not define createX2tModule'));
			return;
		}
		createX2tModule({
			locateFile: function(path) {
				return baseUrl + path;
			},
			print: function() {},
			printErr: function(text) {
				console.warn('[x2t] ' + text);
			},
		}).then(resolve, function(e) {
			reject(error('load', 'x2t.wasm failed to initialize: ' + (e && e.message ? e.message : e)));
		});
	});
	return modulePromise;
}

function removeTree(FS, path) {
	if (!FS.analyzePath(path).exists) {
		return;
	}
	if (FS.isDir(FS.stat(path).mode)) {
		FS.readdir(path).forEach(function(entry) {
			if (entry !== '.' && entry !== '..') {
				removeTree(FS, path + '/' + entry);
			}
		});
		FS.rmdir(path);
	} else {
		FS.unlink(path);
	}
}

function resetWorkDir(FS) {
	removeTree(FS, WORK);
	FS.mkdir(WORK);
	FS.mkdir(WORK + '/fonts');
	FS.mkdir(WORK + '/themes');
	FS.mkdir(WORK + '/media');
}

function writeTree(FS, dir, files) {
	Object.keys(files || {}).forEach(function(name) {
		var parts = name.split('/');
		var current = dir;
		for (var i = 0; i < parts.length - 1; i++) {
			current += '/' + parts[i];
			if (!FS.analyzePath(current).exists) {
				FS.mkdir(current);
			}
		}
		FS.writeFile(dir + '/' + name, new Uint8Array(files[name]));
	});
}

function readTree(FS, dir, prefix, into) {
	FS.readdir(dir).forEach(function(entry) {
		if (entry === '.' || entry === '..') {
			return;
		}
		var path = dir + '/' + entry;
		if (FS.isDir(FS.stat(path).mode)) {
			readTree(FS, path, prefix + entry + '/', into);
		} else {
			into[prefix + entry] = FS.readFile(path, { encoding: 'binary' });
		}
	});
	return into;
}

function extension(fileName) {
	var idx = fileName.lastIndexOf('.');
	return idx === -1 ? '' : fileName.substring(idx + 1).toLowerCase();
}

function sanitizeFileName(fileName) {
	var ext = extension(fileName);
	var base = fileName.substring(0, fileName.length - ext.length - 1)
		.replace(/[\/\\?<>:*|"&'%!{}\[\]\x00-\x1f\x80-\x9f]/g, '')
		.replace(/^\.+$/, '')
		.slice(0, 200) || 'document';
	return base + '.' + ext;
}

function xmlEscape(text) {
	return String(text).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
}

function buildParams(fileFrom, fileTo, formatFrom, formatTo, extra) {
	var xml = '<?xml version="1.0" encoding="utf-8"?>'
		+ '<TaskQueueDataConvert xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema">'
		+ '<m_sFileFrom>' + xmlEscape(fileFrom) + '</m_sFileFrom>'
		+ '<m_sFileTo>' + xmlEscape(fileTo) + '</m_sFileTo>'
		+ '<m_sFontDir>' + WORK + '/fonts/</m_sFontDir>'
		+ '<m_sThemeDir>' + WORK + '/themes</m_sThemeDir>'
		+ '<m_bIsNoBase64>false</m_bIsNoBase64>';
	if (formatFrom) {
		xml += '<m_nFormatFrom>' + formatFrom + '</m_nFormatFrom>';
	}
	if (formatTo) {
		xml += '<m_nFormatTo>' + formatTo + '</m_nFormatTo>';
	}
	if (extra.csv) {
		xml += '<m_nCsvTxtEncoding>' + CSV_UTF8 + '</m_nCsvTxtEncoding>'
			+ '<m_nCsvDelimiter>' + CSV_COMMA + '</m_nCsvDelimiter>';
	}
	if (extra.title) {
		xml += '<m_sTitle>' + xmlEscape(extra.title) + '</m_sTitle>';
	}
	xml += '</TaskQueueDataConvert>';
	return xml;
}

function runX2t(module, fileFrom, fileTo, formatFrom, formatTo, extra) {
	module.FS.writeFile(WORK + '/params.xml', buildParams(fileFrom, fileTo, formatFrom, formatTo, extra));
	var code;
	try {
		code = module.ccall('main1', 'number', ['string'], [WORK + '/params.xml']);
	} catch (e) {
		if (e instanceof RangeError || /memory|allocation/i.test(String(e && e.message))) {
			throw error('memory', 'x2t ran out of memory: ' + (e && e.message ? e.message : e));
		}
		throw error('convert', 'x2t aborted: ' + (e && e.message ? e.message : e));
	}
	if (code !== 0) {
		throw error('convert', 'x2t exited with code ' + code, { exitCode: code });
	}
	if (!module.FS.analyzePath(fileTo).exists) {
		throw error('convert', 'x2t produced no output');
	}
}

function convert(module, request) {
	var FS = module.FS;
	var fileName = sanitizeFileName(request.fileName || 'document.bin');
	var inExt = extension(fileName);
	var to = String(request.to || 'bin').toLowerCase();
	var family = inExt === 'bin' ? request.type : FAMILY[inExt];

	if (inExt !== 'bin' && !FAMILY[inExt]) {
		throw error('unsupported', 'unsupported input format: ' + inExt);
	}
	if (to !== 'bin' && !FORMAT[to]) {
		throw error('unsupported', 'unsupported output format: ' + to);
	}
	if (inExt === 'bin' && !CANVAS[family]) {
		throw error('unsupported', 'type (word|cell|slide) is required for .bin input');
	}
	if (to === 'pdf' && !request.pdfBin) {
		throw error('unsupported', 'to: pdf needs pdfBin, the print output of the editor');
	}

	resetWorkDir(FS);
	writeTree(FS, WORK + '/fonts', request.fonts);
	writeTree(FS, WORK + '/themes', request.themes);
	writeTree(FS, WORK + '/media', request.media);
	FS.writeFile(WORK + '/' + fileName, new Uint8Array(request.data));
	if (request.pdfBin) {
		FS.writeFile(WORK + '/pdf.bin', new Uint8Array(request.pdfBin));
	}

	var extra = { csv: inExt === 'csv' || inExt === 'txt' || to === 'csv' || to === 'txt', title: request.title };
	var fileFrom = WORK + '/' + fileName;
	var fileTo = WORK + '/' + fileName + '.' + to;
	var formatFrom = inExt === 'bin' ? CANVAS[family] : FORMAT[inExt];
	var formatTo = to === 'bin' ? CANVAS[family] : FORMAT[to];

	try {
		runX2t(module, fileFrom, fileTo, formatFrom, formatTo, extra);
	} catch (e) {
		if (!(ODF_TO_OOXML[inExt] && e.code === 'convert')) {
			throw e;
		}
		// Second attempt through the OOXML sibling format
		var ooxml = ODF_TO_OOXML[inExt];
		var intermediate = WORK + '/' + fileName + '.' + ooxml;
		runX2t(module, fileFrom, intermediate, formatFrom, FORMAT[ooxml], extra);
		runX2t(module, intermediate, fileTo, FORMAT[ooxml], formatTo, extra);
	}

	var result = { data: FS.readFile(fileTo, { encoding: 'binary' }), media: {} };
	if (to === 'bin') {
		readTree(FS, WORK + '/media', '', result.media);
	}
	return result;
}

self.onmessage = function(event) {
	var msg = event.data || {};
	if (msg.type === 'init') {
		baseUrl = msg.baseUrl || '';
		loadModule().then(function() {
			self.postMessage({ id: msg.id, ok: true });
		}, function(e) {
			self.postMessage({ id: msg.id, ok: false, error: e });
		});
		return;
	}
	if (msg.type === 'convert') {
		loadModule().then(function(module) {
			var result = convert(module, msg.request);
			var transfer = [result.data.buffer];
			Object.keys(result.media).forEach(function(name) {
				transfer.push(result.media[name].buffer);
			});
			self.postMessage({ id: msg.id, ok: true, result: result }, transfer);
		}).catch(function(e) {
			var err = (e && e.code) ? e : error('convert', String(e && e.message ? e.message : e));
			self.postMessage({ id: msg.id, ok: false, error: err });
		});
		return;
	}
};
