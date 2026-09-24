/*
 * (c) Copyright Ascensio System SIA 2010-2023
 *
 * This program is a free software product. You can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License (AGPL)
 * version 3 as published by the Free Software Foundation. In accordance with
 * Section 7(a) of the GNU AGPL its Section 15 shall be amended to the effect
 * that Ascensio System SIA expressly excludes the warranty of non-infringement
 * of any third-party rights.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR  PURPOSE. For
 * details, see the GNU AGPL at: http://www.gnu.org/licenses/agpl-3.0.html
 *
 * The  interactive user interfaces in modified source and object code versions
 * of the Program must display Appropriate Legal Notices, as required under
 * Section 5 of the GNU AGPL version 3.
 *
 * All the Product's GUI elements, including illustrations and icon sets, as
 * well as technical writing content are licensed under the terms of the
 * Creative Commons Attribution-ShareAlike 4.0 International. See the License
 * terms at http://creativecommons.org/licenses/by-sa/4.0/legalcode
 *
 */

#include "FormattedDiskPagePAPX.h"

namespace DocFileFormat
{
	//an FKP is always one 512 byte page. Its last byte holds crun, preceded by
	//(crun + 1) four byte FCs and crun BX entries.
	static const int FKP_SIZE = 512;
	//the last byte is crun itself, so property data ends one byte earlier
	static const int FKP_DATA_END = FKP_SIZE - 1;
	//highest page number that can be turned into an offset without overflowing
	static const int FKP_MAX_PAGE = 0x7FFFFFFF / FKP_SIZE;

	//copies cb bytes out of the page, zero filling when the source range would
	//leave it. Every offset in an FKP is read from the file and cannot be trusted.
	static void ReadFromPage( unsigned char* dst, const unsigned char* page, int start, int cb )
	{
		if ( ( start >= 0 ) && ( start + cb <= FKP_DATA_END ) )
		{
			memcpy( dst, page + start, cb );
		}
		else
		{
			memset( dst, 0, cb );
		}
	}

	FormattedDiskPagePAPX::~FormattedDiskPagePAPX()
	{
		RELEASEARRAYOBJECTS(rgfc);
		RELEASEARRAYOBJECTS(rgbx);

		if (grppapx != NULL)
		{
			for (unsigned int i = 0; i < grppapxSize; i++)
			{
				RELEASEOBJECT(grppapx[i]);
			}

			RELEASEARRAYOBJECTS(grppapx);
		}
	}

	/*========================================================================================================*/

	FormattedDiskPagePAPX::FormattedDiskPagePAPX(POLE::Stream* wordStream, int offset, POLE::Stream* dataStream, int nWordVersion, bool fComplex) :
		FormattedDiskPage(), rgbx(NULL), grppapxSize(0), grppapx(NULL)
	{
		Type = Paragraph;
		WordStream = wordStream;

		//read the 512 bytes (FKP)
		//the page is zeroed first: an offset past the end of the stream makes the
		//read return nothing at all, and the parser must not go on to interpret
		//uninitialised heap as offsets and lengths
		unsigned char* bytes = NULL;
		bytes = new unsigned char[FKP_SIZE];
		memset(bytes, 0, FKP_SIZE);

		//the offset is derived from a page number stored in the file. A page that
		//lies outside the stream reads short, and is skipped by re-zeroing the
		//buffer, which leaves crun at 0 and parses nothing
		if (offset >= 0)
		{
			WordStream->seek(offset);

			if (WordStream->read(bytes, FKP_SIZE) != FKP_SIZE)
			{
				memset(bytes, 0, FKP_SIZE);
			}
		}

		//a BX is one offset byte plus a paragraph height. Word 2 keeps the height
		//in the property area instead, so only the offset byte is walked there.
		int bxSize = 1;

		if (fComplex || nWordVersion == 0)
		{
			bxSize = 1 + 12;
		}
		else if (nWordVersion != 2)
		{
			bxSize = 1 + 6;
		}

		//get the count, capped at what a single page can describe
		crun = bytes[FKP_SIZE - 1];

		int maxCrun = ( FKP_SIZE - 1 - 4 ) / ( 4 + bxSize );

		if (crun > maxCrun)
		{
			crun = (unsigned char)maxCrun;
		}

		//create and fill the array with the adresses
		rgfcSize = crun + 1;
		rgfc = new int[rgfcSize];

		int j = 0;

		for (unsigned int i = 0; i < rgfcSize; i++)
		{
			rgfc[i] = FormatUtils::BytesToInt32(bytes, j, 512);
			j += 4;
		}
		rgbx = new BX[crun];
		grppapxSize = crun;
		grppapx = new ParagraphPropertyExceptions * [grppapxSize];

		for (unsigned int i = 0; i < grppapxSize; i++)
		{
			grppapx[i] = NULL;
		}

		j = 4 * (crun + 1);

		unsigned char phe[12];

		for (unsigned char i = 0; i < crun; i++)
		{
			BX bx;
			bx.wordOffset = bytes[j];
			j++;

			if (fComplex || nWordVersion == 0)
			{
				ReadFromPage(phe, bytes, j, 12);

				//fill the rgbx array
				bx.phe = ParagraphHeight(phe, 12, false);

				j += 12;
			}
			else if (nWordVersion == 2)
			{
				//this one combines both file controlled offsets, so it can point
				//a long way outside the page
				ReadFromPage(phe, bytes, bx.wordOffset * 2 + j + 1, 6);

				//fill the rgbx array
				bx.phe = ParagraphHeight(phe, 6, false);
			}
			else
			{
				ReadFromPage(phe, bytes, j, 6);

				//fill the rgbx array
				bx.phe = ParagraphHeight(phe, 6, false);

				j += 6;
			}
			rgbx[i] = bx;

			if (bx.wordOffset != 0)
			{
				unsigned char padbyte = 0;
				//wordOffset is a byte, so the count byte is always inside the page
				int papxStart = bx.wordOffset * 2;
				unsigned char cw = bytes[papxStart];
				//if that unsigned char is zero, it's a pad unsigned char, and the word count is the following unsigned char
				if (cw == 0)
				{
					padbyte = 1;
					cw = ( papxStart + 1 < FKP_DATA_END ) ? bytes[papxStart + 1] : 0;
				}
				if (cw != 0)
				{
					int sz = cw * 2;

					//the properties themselves must not run past the property area
					if (papxStart + padbyte + 1 + sz <= FKP_DATA_END)
					{
						//read the bytes for papx
						unsigned char* papx = new unsigned char[sz];
						memcpy(papx, (bytes + papxStart + padbyte + 1), sz);

						//parse PAPX and fill grppapx
						grppapx[i] = new ParagraphPropertyExceptions(papx, sz, dataStream, nWordVersion);

						RELEASEARRAYOBJECTS(papx);
					}
				}

			}
			else
			{
				//create a PAPX which doesn't modify anything
				grppapx[i] = new ParagraphPropertyExceptions();
			}
		}

		RELEASEARRAYOBJECTS(bytes);
	}

	/*========================================================================================================*/

	/// Parses the 0Table (or 1Table) for FKP _entries containing PAPX
	std::vector<FormattedDiskPagePAPX*>* FormattedDiskPagePAPX::GetAllPAPXFKPs(FileInformationBlock* fib, POLE::Stream* wordStream, POLE::Stream* tableStream, POLE::Stream* dataStream)
	{
		std::vector<FormattedDiskPagePAPX*>* PAPXlist = new std::vector<FormattedDiskPagePAPX*>();

		if ((fib->m_FibWord97.lcbPlcfBtePapx < 8 && fib->m_nWordVersion > 0 && fib->m_FibBase.fComplex == false) || fib->m_FibWord97.lcbPlcfBtePapx < 4) return PAPXlist;

		//get bintable for PAPX
		unsigned char* binTablePapx = new unsigned char[fib->m_FibWord97.lcbPlcfBtePapx + 1];

		if (tableStream)
		{
			tableStream->seek(fib->m_FibWord97.fcPlcfBtePapx);
			tableStream->read(binTablePapx, fib->m_FibWord97.lcbPlcfBtePapx);
		}

		//there are n offsets and n-1 fkp's in the bin table

		if (fib->m_nWordVersion > 0 && fib->m_FibBase.fComplex == false)
		{
			int	n = (((int)fib->m_FibWord97.lcbPlcfBtePapx - 8) / 6) + 1;

			unsigned int first = FormatUtils::BytesToInt32(binTablePapx, 0, fib->m_FibWord97.lcbPlcfBtePapx);
			unsigned int last = FormatUtils::BytesToInt32(binTablePapx, 4, fib->m_FibWord97.lcbPlcfBtePapx);

			int start_papx = 8;
			if (fib->m_FibWord97.lcbPlcfBtePapx - 8 > (n - 1) * 4)
			{
				start_papx += ((n - 1) * 4); //crun duplication
			}

			int offset = 0;
			for (unsigned int i = start_papx; i < fib->m_FibWord97.lcbPlcfBtePapx; i += 2)
			{
				//indexed FKP is the xth 512byte page
				int fkpnr = FormatUtils::BytesToInt16(binTablePapx, i, fib->m_FibWord97.lcbPlcfBtePapx);

				//the page number comes from the file and is signed, skip the ones
				//that cannot address a real page
				if ( fkpnr < 0 || fkpnr > FKP_MAX_PAGE ) continue;

				//so starts at:
				int offset = fkpnr * FKP_SIZE;

				//parse the FKP and add it to the list
				PAPXlist->push_back(new FormattedDiskPagePAPX(wordStream, offset, dataStream, fib->m_nWordVersion, fib->m_FibBase.fComplex));
			}

			//if (PAPXlist->back()->rgfc[PAPXlist->back()->rgfcSize-1] < last)
			//{
			//	PAPXlist->back()->rgfc[PAPXlist->back()->rgfcSize-1] = last;
			//	//tableStream->read( binTablePapx, fib->m_FibWord97.lcbPlcfBtePapx);
			//	//offset+=512;
			//	//PAPXlist->push_back( new FormattedDiskPagePAPX( wordStream, offset, dataStream ) );
			//}
		}
		else
		{
			int n = (((int)fib->m_FibWord97.lcbPlcfBtePapx - 4) / 8) + 1;
			//Get the indexed PAPX FKPs
			for (unsigned int i = (n * 4); i < fib->m_FibWord97.lcbPlcfBtePapx; i += 4)
			{
				//indexed FKP is the xth 512byte page
				int fkpnr = FormatUtils::BytesToInt32(binTablePapx, i, fib->m_FibWord97.lcbPlcfBtePapx);

				//the page number comes from the file and is signed, skip the ones
				//that cannot address a real page
				if ( fkpnr < 0 || fkpnr > FKP_MAX_PAGE ) continue;

				//so starts at:
				int offset = fkpnr * FKP_SIZE;

				//parse the FKP and add it to the list
				PAPXlist->push_back(new FormattedDiskPagePAPX(wordStream, offset, dataStream, fib->m_nWordVersion, fib->m_FibBase.fComplex));
			}

		}

		RELEASEARRAYOBJECTS(binTablePapx);

		return PAPXlist;
	}

	/*========================================================================================================*/

	/// Returns a list of all PAPX FCs between they given boundaries.
	std::vector<int>* FormattedDiskPagePAPX::GetFileCharacterPositions
	(
		int fcMin,
		int fcMax,
		FileInformationBlock* fib,
		POLE::Stream* wordStream,
		POLE::Stream* tableStream,
		POLE::Stream* dataStream
	)
	{
		std::vector<int>* cpList = new std::vector<int>();
		std::vector<FormattedDiskPagePAPX*>* fkps = FormattedDiskPagePAPX::GetAllPAPXFKPs(fib, wordStream, tableStream, dataStream);
		unsigned int i = 0;
		FormattedDiskPagePAPX* fkp = NULL;

		for (size_t i = 0; i < fkps->size(); ++i)
		{
			fkp = fkps->at(i);

			//the last entry of each is always the same as the first entry of the next FKP
			//so, ignore all last _entries except for the last FKP.
			int max_ = fkp->rgfcSize;

			if (i++ < fkps->size() - 1)
			{
				max_--;
			}

			for (int j = 0; j < max_; j++)
			{
				if ((fkp->rgfc[j] >= fcMin) && (fkp->rgfc[j] < fcMax))
				{
					cpList->push_back(fkp->rgfc[j]);
				}
			}

			RELEASEOBJECT(fkp);
		}

		fkps->clear();
		RELEASEOBJECT(fkps);

		return cpList;
	}

	/*========================================================================================================*/

	/// Returnes a list of all ParagraphPropertyExceptions which correspond to text 
	/// between the given offsets.
	std::vector<ParagraphPropertyExceptions*>* FormattedDiskPagePAPX::GetParagraphPropertyExceptions
	(
		int fcMin,
		int fcMax,
		FileInformationBlock* fib,
		POLE::Stream* wordStream,
		POLE::Stream* tableStream,
		POLE::Stream* dataStream
	)
	{
		std::vector<ParagraphPropertyExceptions*>* ppxList = new std::vector<ParagraphPropertyExceptions*>();
		std::vector<FormattedDiskPagePAPX*>* fkps = FormattedDiskPagePAPX::GetAllPAPXFKPs(fib, wordStream, tableStream, dataStream);

		for (size_t i = 0; i < fkps->size(); ++i)
		{
			FormattedDiskPagePAPX* fkp = fkps->at(i);

			for (unsigned int j = 0; j < fkp->grppapxSize; j++)
			{
				if ((fkp->rgfc[j] >= fcMin) && (fkp->rgfc[j] < fcMax))
				{
					ppxList->push_back(fkp->grppapx[j]);
				}
			}

			RELEASEOBJECT(fkp);

			fkps->clear();
			RELEASEOBJECT(fkps);
		}
		return ppxList;
	}
}