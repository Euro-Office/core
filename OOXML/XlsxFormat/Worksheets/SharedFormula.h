/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
#pragma once

// Detection of vertical runs of "the same formula shifted by one row", for the xlsx -> Editor.bin fast path.
//
// Many generators (Google Sheets among them) write no shared formulas, so the browser would decode every formula
// as text and call compareFormula with the cell above to rediscover the sharing. x2t can find the runs while it
// streams the sheet and write them as shared formulas instead. See CSharedFormulaRows in SheetData.cpp.
//
// The unit here is a "row-relative key": the formula text with every relative row number replaced by its distance
// from the cell's own row. Two cells in the same column can share a formula exactly when their keys are equal.
// Everything that is not a plain A1 cell reference must be equal character by character, and whenever the text
// cannot be classified with certainty the key is refused, because a wrongly shared formula changes values.
//
// The rewrites sdkjs applies to a formula before it compares formulas (Serialize.js, setFormulaOpen) are
// applied here too, so both sides agree on what the formula is.

#include "../../../DesktopEditor/common/Types.h"

#include <cstddef>
#include <cwchar>
#include <string>

namespace OOX
{
	namespace Spreadsheet
	{
		namespace SharedFormula
		{
			typedef std::basic_string<WCHAR> Key;

			// Excel limits, the same ones sdkjs checks when it reads a cell reference.
			const _UINT32 c_nMaxRow = 1048576;
			const _UINT32 c_nMaxCol = 16384;
			// sdkjs does not parse a formula longer than 8192 characters and rejects a single reference longer than
			// 2048. A shared master that fails to parse would silently turn all its followers into constants, so
			// longer formulas are never grouped. Staying under the smaller limit covers both.
			const LONG c_nMaxFormulaLength = 2048;
			// Excel and sdkjs write shared formulas in blocks of 64 rows. A master can only state its range up front,
			// so x2t uses the same block size: the range is known when the master is written.
			const _UINT32 c_nBlockRows = 64;
			// Shared formula ids written by x2t start here, far above the small sequential ids that Excel writes,
			// so they cannot collide with the ids of shared formulas that are already in the sheet.
			const _INT32 c_nFirstSi = 0x40000000;

			inline bool IsLetter(WCHAR c)
			{
				return (c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z');
			}
			inline bool IsDigit(WCHAR c)
			{
				return c >= L'0' && c <= L'9';
			}
			// A character that can be part of a defined name or a sheet name written without quotes.
			inline bool IsNameChar(WCHAR c)
			{
				return IsLetter(c) || IsDigit(c) || c == L'_' || c == L'.' || c == L'\\' || c == L'?' || c >= 0x80;
			}
			inline bool IsSpace(WCHAR c)
			{
				return c == L' ' || c == L'\t' || c == L'\n' || c == L'\r';
			}

			// Writes a column (0-based) and a row range (1-based) as "B2:B65" into pBuffer (at least 24 characters).
			inline LONG WriteColumnRange(_UINT32 nCol, _UINT32 nFirstRow, _UINT32 nLastRow, WCHAR* pBuffer)
			{
				WCHAR sCol[4];
				int nLetters = 0;
				_UINT32 nValue = nCol + 1;
				while (nValue > 0 && nLetters < 3)
				{
					--nValue;
					sCol[nLetters++] = (WCHAR)(L'A' + nValue % 26);
					nValue /= 26;
				}
				LONG nLen = 0;
				for (int part = 0; part < 2; ++part)
				{
					for (int i = nLetters - 1; i >= 0; --i)
						pBuffer[nLen++] = sCol[i];
					WCHAR sRow[12];
					int nDigits = 0;
					_UINT32 nRow = part == 0 ? nFirstRow : nLastRow;
					do
					{
						sRow[nDigits++] = (WCHAR)(L'0' + nRow % 10);
						nRow /= 10;
					} while (nRow > 0);
					for (int i = nDigits - 1; i >= 0; --i)
						pBuffer[nLen++] = sRow[i];
					if (part == 0)
						pBuffer[nLen++] = L':';
				}
				return nLen;
			}

			class CKeyBuilder
			{
			public:
				// Builds the row-relative key of a formula that sits in row nRow (1-based).
				// Returns false when the formula must not take part in sharing.
				bool Build(const WCHAR* pFormula, LONG nLen, _UINT32 nRow, Key& key)
				{
					if (nLen <= 0 || nLen > c_nMaxFormulaLength)
						return false;
					const WCHAR* pText = pFormula;
					size_t nTextLen = (size_t)nLen;
					if (!ApplyOpenRewrites(pText, nTextLen))
						return false;
					return Tokenize(pText, nTextLen, nRow, key);
				}

			private:
				Key m_sRewritten;

				static bool StartsWith(const WCHAR* p, size_t n, const wchar_t* sLiteral)
				{
					size_t i = 0;
					for (; sLiteral[i] != 0; ++i)
					{
						if (i >= n || p[i] != sLiteral[i])
							return false;
					}
					return true;
				}

				// sdkjs, setFormulaOpen: removes "_xludf." and a leading "=", and unwraps Google Sheets'
				//   IFERROR(__xludf.DUMMYFUNCTION("<formula>"),<cached value>)
				// to <formula>, dropping the cached value. Formulas for which the rewrite is not worth
				// reproducing are left out of sharing.
				bool ApplyOpenRewrites(const WCHAR*& p, size_t& n)
				{
					static const wchar_t sXludf[] = L"_xludf.";
					static const wchar_t sWrapper[] = L"IFERROR(__xludf.DUMMYFUNCTION(\"";
					const size_t nWrapper = sizeof(sWrapper) / sizeof(wchar_t) - 1;

					if (StartsWith(p, n, sXludf))
						return false;
					if (StartsWith(p, n, sWrapper))
					{
						// formula.v.substr(0, formula.v.lastIndexOf('")')), then every "" becomes "
						size_t nEnd = (size_t)-1;
						for (size_t i = n; i > nWrapper + 1; --i)
						{
							if (p[i - 2] == L'"' && p[i - 1] == L')')
							{
								nEnd = i - 2;
								break;
							}
						}
						if (nEnd == (size_t)-1)
							return false;
						// sdkjs cuts at the last '")'. That is the end of the wrapped formula only when the cached value
						// after it holds no quote. With a text value, IFERROR(...DUMMYFUNCTION("A1+1"),"x"), the cut lands at
						// the end of "x" and sdkjs is left with a broken formula it cannot parse or shift. Sharing such cells
						// would show every cell below the master with the master's references, so they stay as they are.
						const size_t nRealEnd = SkipQuoted(p, n, nWrapper - 1);
						if (nRealEnd == 0 || nRealEnd - 1 != nEnd)
							return false;
						m_sRewritten.clear();
						m_sRewritten.reserve(nEnd - nWrapper);
						for (size_t i = nWrapper; i < nEnd; ++i)
						{
							m_sRewritten.push_back(p[i]);
							if (p[i] == L'"' && i + 1 < nEnd && p[i + 1] == L'"')
								++i;
						}
						p = m_sRewritten.data();
						n = m_sRewritten.size();
					}
					return n > 0 && p[0] != L'=';
				}

				static void AppendNumber(Key& key, long nValue)
				{
					if (nValue < 0)
					{
						key.push_back(L'-');
						nValue = -nValue;
					}
					WCHAR sDigits[24];
					int nDigits = 0;
					do
					{
						sDigits[nDigits++] = (WCHAR)(L'0' + nValue % 10);
						nValue /= 10;
					} while (nValue > 0);
					while (nDigits > 0)
						key.push_back(sDigits[--nDigits]);
				}

				// Copies a "..." or '...' run, where the quote character is doubled to escape itself.
				// Returns the index after the closing quote, or 0 when the run is not closed.
				static size_t SkipQuoted(const WCHAR* s, size_t n, size_t i)
				{
					const WCHAR cQuote = s[i];
					size_t j = i + 1;
					for (;;)
					{
						if (j >= n)
							return 0;
						if (s[j] == cQuote)
						{
							if (j + 1 < n && s[j + 1] == cQuote)
							{
								j += 2;
								continue;
							}
							return j + 1;
						}
						++j;
					}
				}

				bool Tokenize(const WCHAR* s, size_t n, _UINT32 nRow, Key& key)
				{
					key.clear();
					key.reserve(n + 16);
					size_t i = 0;
					// The end and the row of the last cell reference, to recognise "A1:B2".
					size_t nLastRefEnd = (size_t)-1;
					_UINT32 nLastRefRow = 0;
					while (i < n)
					{
						const WCHAR c = s[i];
						if (c == L'"')
						{
							size_t j = SkipQuoted(s, n, i);
							if (j == 0)
								return false;
							key.append(s + i, j - i);
							i = j;
						}
						else if (c == L'\'')
						{
							// A quoted sheet name, which must be followed by '!'.
							size_t j = SkipQuoted(s, n, i);
							if (j == 0 || j >= n || s[j] != L'!')
								return false;
							key.append(s + i, j + 1 - i);
							i = j + 1;
						}
						else if (c == L'[')
						{
							// A structured reference or an external workbook index, with nesting and ' as the escape.
							size_t j = i;
							int nDepth = 0;
							for (; j < n; ++j)
							{
								if (s[j] == L'\'')
								{
									++j;
								}
								else if (s[j] == L'[')
								{
									++nDepth;
								}
								else if (s[j] == L']' && --nDepth == 0)
								{
									break;
								}
							}
							if (j >= n)
								return false;
							key.append(s + i, j + 1 - i);
							i = j + 1;
						}
						else if (c == L'{')
						{
							// An array constant holds numbers, strings and separators only.
							size_t j = i + 1;
							while (j < n && s[j] != L'}')
							{
								if (s[j] == L'"')
								{
									j = SkipQuoted(s, n, j);
									if (j == 0)
										return false;
								}
								else
								{
									++j;
								}
							}
							if (j >= n)
								return false;
							key.append(s + i, j + 1 - i);
							i = j + 1;
						}
						else if (c == L'#')
						{
							size_t j = i + 1;
							const bool bSpill = i > 0 && (IsNameChar(s[i - 1]) || s[i - 1] == L')' || s[i - 1] == L']' || s[i - 1] == L'}');
							if (!bSpill)
							{
								// #N/A, #REF!, #DIV/0!, #NAME?
								while (j < n && (IsLetter(s[j]) || IsDigit(s[j]) || s[j] == L'_' || s[j] == L'/'))
									++j;
								if (j < n && (s[j] == L'!' || s[j] == L'?'))
									++j;
							}
							key.append(s + i, j - i);
							i = j;
						}
						else if (IsDigit(c) || (c == L'.' && i + 1 < n && IsDigit(s[i + 1])))
						{
							size_t j = i;
							bool bPlainInteger = true;
							while (j < n && IsDigit(s[j]))
								++j;
							if (j < n && s[j] == L'.')
							{
								bPlainInteger = false;
								++j;
								while (j < n && IsDigit(s[j]))
									++j;
							}
							if (j < n && (s[j] == L'E' || s[j] == L'e'))
							{
								size_t k = j + 1;
								if (k < n && (s[k] == L'+' || s[k] == L'-'))
									++k;
								if (k < n && IsDigit(s[k]))
								{
									bPlainInteger = false;
									j = k;
									while (j < n && IsDigit(s[j]))
										++j;
								}
							}
							// "1:3" is a reference to whole rows, and "2D" or "1.5.2" is not a number: leave them alone.
							if (j < n && (IsNameChar(s[j]) || s[j] == L'$'))
								return false;
							if (bPlainInteger && j < n && s[j] == L':')
								return false;
							key.append(s + i, j - i);
							i = j;
						}
						else if (IsLetter(c) || c == L'_' || c == L'\\' || c == L'$' || c >= 0x80)
						{
							size_t j = i;
							while (j < n && (IsNameChar(s[j]) || s[j] == L'$'))
								++j;
							if (!Token(s, i, j, n, nRow, key, nLastRefEnd, nLastRefRow))
								return false;
							i = j;
						}
						else if (IsSpace(c) || c == L'+' || c == L'-' || c == L'*' || c == L'/' || c == L'^' || c == L'&' ||
								 c == L'=' || c == L'<' || c == L'>' || c == L',' || c == L';' || c == L'(' || c == L')' ||
								 c == L':' || c == L'%' || c == L'@' || c == L'!')
						{
							key.push_back(c);
							++i;
						}
						else
						{
							return false;
						}
					}
					return true;
				}

				// A run of name characters s[i, j). A function, a sheet, a table or a name is copied as it is;
				// a cell reference has its row replaced by the distance to nRow if the row is relative.
				static bool Token(const WCHAR* s, size_t i, size_t j, size_t n, _UINT32 nRow, Key& key, size_t& nLastRefEnd,
								  _UINT32& nLastRefRow)
				{
					const WCHAR cNext = j < n ? s[j] : 0;
					if (cNext == L'(' || cNext == L'!' || cNext == L'[')
					{
						key.append(s + i, j - i);
						return true;
					}

					bool bHasDollar = false;
					for (size_t k = i; k < j; ++k)
					{
						if (s[k] == L'$')
							bHasDollar = true;
					}

					size_t p = i;
					if (s[p] == L'$')
						++p;
					const size_t nColStart = p;
					while (p < j && IsLetter(s[p]))
						++p;
					const size_t nColLetters = p - nColStart;

					if (nColLetters == 0 || nColLetters > 3)
					{
						// A name such as Rate, _xlfn.X or Sales2024: fine, unless a '$' makes it look like a reference.
						if (bHasDollar)
							return false;
						key.append(s + i, j - i);
						return true;
					}

					size_t q = p;
					bool bAbsRow = false;
					if (q < j && s[q] == L'$')
					{
						bAbsRow = true;
						++q;
					}
					const size_t nRowStart = q;
					while (q < j && IsDigit(s[q]))
						++q;
					const size_t nRowDigits = q - nRowStart;

					if (q == j && nRowDigits > 0)
					{
						// A cell reference. Anything the reader would not accept as one is refused, not guessed at.
						if (s[nRowStart] == L'0' || nRowDigits > 7)
							return false;
						_UINT32 nColumn = 0;
						for (size_t k = nColStart; k < nColStart + nColLetters; ++k)
							nColumn = nColumn * 26 + (_UINT32)((s[k] | 0x20) - L'a' + 1);
						_UINT32 nRefRow = 0;
						for (size_t k = nRowStart; k < q; ++k)
							nRefRow = nRefRow * 10 + (_UINT32)(s[k] - L'0');
						if (nColumn > c_nMaxCol || nRefRow > c_nMaxRow)
							return false;
						// A range that runs upwards ("B$2:B1") is turned around by sdkjs when it reads it, which also swaps
						// which end is absolute. A formula shared from such a master is shifted wrongly, and sdkjs itself
						// does not share these cells. Refuse every reversed range, wherever it is.
						if (nLastRefEnd != (size_t)-1 && nLastRefEnd + 1 == i && s[nLastRefEnd] == L':' && nLastRefRow > nRefRow)
							return false;
						nLastRefEnd = j;
						nLastRefRow = nRefRow;
						if (bAbsRow)
						{
							key.append(s + i, j - i);
						}
						else
						{
							key.append(s + i, nRowStart - i);
							key.push_back(L'\x01');
							AppendNumber(key, (long)nRefRow - (long)nRow);
							key.push_back(L'\x02');
						}
						return true;
					}
					if (nRowDigits > 0 || bHasDollar)
					{
						// "A1_x" or "A$": the start looks like a reference, the rest does not.
						// "A:A" and "$A:$C" never get here: a column alone has no '$' after its letters.
						if (q == j && nRowDigits == 0 && !bAbsRow && s[i] == L'$' && p == j)
						{
							key.append(s + i, j - i);
							return true;
						}
						return false;
					}
					// A column letters alone ("A" in A:A), or a name that starts with a few letters ("NA", "AB_C").
					key.append(s + i, j - i);
					return true;
				}
			};
		}
	}
}
