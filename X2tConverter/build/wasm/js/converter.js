/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 * Browser API of the x2t WebAssembly converter. Conversions run in a Web
 * Worker (x2t-worker.js) that loads x2t.js/x2t.wasm from the same directory
 * as this module, so the calling page never blocks on a conversion.
 *
 *   import { createConverter } from '<document server>/x2t/converter.js';
 *   const x2t = await createConverter();
 *   const { data, media } = await x2t.convert({ data: bytes, fileName: 'report.docx', to: 'bin' });
 *   x2t.terminate();
 *
 * See README.md for the full request and error contract.
 */

const WORKER_FILE = 'x2t-worker.js';

function toArrayBuffer(input) {
	if (input instanceof ArrayBuffer) {
		return Promise.resolve(input);
	}
	if (ArrayBuffer.isView(input)) {
		return Promise.resolve(input.buffer.slice(input.byteOffset, input.byteOffset + input.byteLength));
	}
	if (typeof Blob !== 'undefined' && input instanceof Blob) {
		return input.arrayBuffer();
	}
	return Promise.reject(new Error('data must be an ArrayBuffer, a typed array or a Blob'));
}

async function toBufferMap(map) {
	const out = {};
	for (const name of Object.keys(map || {})) {
		const value = map[name];
		if (typeof value === 'string') {
			const response = await fetch(value);
			if (!response.ok) {
				throw Object.assign(new Error(`could not fetch ${name}: HTTP ${response.status}`), { code: 'load' });
			}
			out[name] = await response.arrayBuffer();
		} else {
			out[name] = await toArrayBuffer(value);
		}
	}
	return out;
}

function converterError(error) {
	const e = new Error(error && error.message ? error.message : String(error));
	e.code = (error && error.code) || 'convert';
	if (error && error.exitCode !== undefined) {
		e.exitCode = error.exitCode;
	}
	return e;
}

/**
 * @param {object} [options]
 * @param {string} [options.baseUrl] directory holding x2t.js, x2t.wasm and x2t-worker.js; defaults to this module's directory
 * @return {Promise<{convert: Function, terminate: Function}>}
 */
export async function createConverter(options = {}) {
	const baseUrl = new URL(options.baseUrl || './', import.meta.url).href;
	const worker = new Worker(new URL(WORKER_FILE, baseUrl));
	const pending = new Map();
	let nextId = 1;
	let terminated = false;

	worker.onmessage = (event) => {
		const msg = event.data || {};
		const entry = pending.get(msg.id);
		if (!entry) {
			return;
		}
		pending.delete(msg.id);
		if (msg.ok) {
			entry.resolve(msg.result);
		} else {
			entry.reject(converterError(msg.error));
		}
	};
	worker.onerror = (event) => {
		const e = converterError({ code: 'load', message: event.message || 'x2t worker failed' });
		for (const entry of pending.values()) {
			entry.reject(e);
		}
		pending.clear();
	};

	const send = (type, payload, transfer) => new Promise((resolve, reject) => {
		if (terminated) {
			reject(converterError({ code: 'load', message: 'converter was terminated' }));
			return;
		}
		const id = nextId++;
		pending.set(id, { resolve, reject });
		worker.postMessage({ id, type, ...payload }, transfer || []);
	});

	await send('init', { baseUrl });

	return {
		/**
		 * @param {object} request
		 * @param {ArrayBuffer|Uint8Array|Blob} request.data the document
		 * @param {string} request.fileName name with the extension that decides the input format
		 * @param {string} [request.to='bin'] bin | docx | xlsx | pptx | pdf | odt | ods | odp | csv | txt | ...
		 * @param {object} [request.fonts] { fileName: ArrayBuffer | url } written to /working/fonts
		 * @param {object} [request.themes] { path: ArrayBuffer } written to /working/themes
		 * @param {object} [request.media] { name: ArrayBuffer } written to /working/media (export)
		 * @param {string} [request.type] word | cell | slide, required when fileName ends in .bin
		 * @param {ArrayBuffer|Uint8Array|Blob} [request.pdfBin] the editor's print output, required for to === 'pdf'
		 * @param {string} [request.title] document title for formats that carry one
		 * @return {Promise<{data: Uint8Array, media: Object<string, Uint8Array>}>}
		 */
		async convert(request) {
			const data = await toArrayBuffer(request.data);
			const fonts = await toBufferMap(request.fonts);
			const themes = await toBufferMap(request.themes);
			const media = await toBufferMap(request.media);
			const pdfBin = request.pdfBin ? await toArrayBuffer(request.pdfBin) : undefined;
			const transfer = [data, ...Object.values(fonts), ...Object.values(themes), ...Object.values(media)];
			if (pdfBin) {
				transfer.push(pdfBin);
			}
			return send('convert', {
				request: {
					data,
					fileName: request.fileName,
					to: request.to || 'bin',
					type: request.type,
					title: request.title,
					fonts,
					themes,
					media,
					pdfBin,
				},
			}, transfer);
		},

		terminate() {
			terminated = true;
			worker.terminate();
			const e = converterError({ code: 'load', message: 'converter was terminated' });
			for (const entry of pending.values()) {
				entry.reject(e);
			}
			pending.clear();
		},
	};
}
