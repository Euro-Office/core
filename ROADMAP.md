<!--
SPDX-FileCopyrightText: 2026 Euro-Office contributors
SPDX-License-Identifier: CC0-1.0
-->

# Euro-Office Integration Roadmap

> This roadmap covers the integration of Euro-Office DocumentServer with external platforms (OpenCloud, SOGo, Open-Xchange, Zimbra) via WOPI and Docs API protocols.
>
> Full integration planning document: [.github/INTEGRATION-PLANNING.md](https://github.com/Euro-Office/DocumentServer/blob/main/.github/INTEGRATION-PLANNING.md)

## Role in Integration

This repo contains the **document processing engine** — the low-level C++ libraries for parsing, rendering, and converting document formats. It handles file format conversion that all platform integrations depend on.

## Roadmap Items

### Phase 1 — Foundation

- [ ] Verify file format conversion reliability for integration workflows
  - OOXML (DOCX, XLSX, PPTX) ↔ ODF (ODT, ODS, ODP)
  - OOXML/ODF ↔ PDF
  - Legacy formats (DOC, XLS, PPT, RTF)
- [ ] Test conversion edge cases common in platform integrations
  - Large files, complex formatting, embedded objects
  - Cross-platform encoding issues

### Shared Requirements (this repo)

- [ ] **File conversion** — On-the-fly format conversion for all platform integrations

---

## Cross-Repo References

| Repo | Role |
|------|------|
| [server](https://github.com/Euro-Office/server) | WOPI endpoints, discovery, proof keys, JWT, admin panel |
| [sdkjs](https://github.com/Euro-Office/sdkjs) | Docs API, editor engine, co-editing, format conversion |
| [web-apps](https://github.com/Euro-Office/web-apps) | Editor UI, toolbar, plugins, branding |
| [core](https://github.com/Euro-Office/core) | File format conversion engine (OOXML, ODF, PDF) |
| [eurooffice-nextcloud](https://github.com/Euro-Office/eurooffice-nextcloud) | OpenCloud/Nextcloud WOPI connector |
| [DocumentServer](https://github.com/Euro-Office/DocumentServer) | Integration orchestration, Docker, Helm |
| [document-server-integration](https://github.com/Euro-Office/document-server-integration) | Integration examples & API docs |
| [document-server-package](https://github.com/Euro-Office/document-server-package) | DEB/RPM/Helm packaging |
| [docker-ci](https://github.com/Euro-Office/docker-ci) | Docker images for deployment |
| [desktop-sdk](https://github.com/Euro-Office/desktop-sdk) | Desktop CEF integration |
| [desktop-apps](https://github.com/Euro-Office/desktop-apps) | Desktop packaging & Electron wrappers |

## Protocol Documentation

- [WOPI Protocol Overview — Microsoft Learn](https://learn.microsoft.com/en-us/microsoft-365/cloud-storage-partner-program/online/)
- [WOPI Overview — ONLYOFFICE API](https://api.onlyoffice.com/docs/docs-api/using-wopi/overview/)
- [WOPI REST API — ONLYOFFICE API](https://api.onlyoffice.com/docs/docs-api/using-wopi/wopi-rest-api/)
- [WOPI Discovery — ONLYOFFICE API](https://api.onlyoffice.com/docs/docs-api/using-wopi/wopi-discovery/)
- [Docs API Basic Concepts — ONLYOFFICE API](https://api.onlyoffice.com/docs/docs-api/get-started/basic-concepts/)
