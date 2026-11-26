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
#include "formulasconvert.h"

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

#include"../../OOXML/Base/Unit.h"
#include "../Reader/Converter/xlsxconversioncontext.h"
#include "../Reader/Converter/xlsx_utils.h"

namespace cpdoccore {
namespace formulasconvert {

	namespace
	{
		// The OOXML name as Excel stores it, the OpenFormula name as LibreOffice writes it
		typedef std::unordered_map<std::wstring, std::wstring> names_map;

		void add_names(names_map& ooxml_by_odf, const std::wstring& odf_prefix, const std::wstring& ooxml_prefix, std::initializer_list<const wchar_t*> names)
		{
			for (const wchar_t* name : names)
				ooxml_by_odf[odf_prefix + name] = ooxml_prefix + name;
		}

		const names_map& ooxml_by_odf_name()
		{
			static const names_map names = []
			{
				names_map n;
				add_names(n, L"COM.MICROSOFT.", L"_xlfn.", {
					L"AGGREGATE", L"BETA.DIST", L"BETA.INV", L"BINOM.DIST", L"BINOM.INV", L"CEILING.MATH", L"CEILING.PRECISE",
					L"CHISQ.DIST", L"CHISQ.DIST.RT", L"CHISQ.INV", L"CHISQ.INV.RT", L"CHISQ.TEST", L"CHOOSECOLS", L"CHOOSEROWS",
					L"CONCAT", L"CONFIDENCE.NORM", L"CONFIDENCE.T", L"COVARIANCE.P", L"COVARIANCE.S", L"DROP", L"ENCODEURL",
					L"ERF.PRECISE", L"ERFC.PRECISE", L"EXPAND", L"EXPON.DIST", L"F.DIST.RT", L"F.INV.RT", L"F.TEST", L"FILTERXML",
					L"FLOOR.MATH", L"FLOOR.PRECISE", L"FORECAST.ETS", L"FORECAST.ETS.CONFINT", L"FORECAST.ETS.SEASONALITY",
					L"FORECAST.ETS.STAT", L"FORECAST.LINEAR", L"GAMMA.DIST", L"GAMMA.INV", L"GAMMALN.PRECISE", L"HSTACK",
					L"HYPGEOM.DIST", L"IFS", L"LET", L"LOGNORM.DIST", L"LOGNORM.INV", L"MAXIFS", L"MINIFS", L"MODE.MULT",
					L"MODE.SNGL", L"NEGBINOM.DIST", L"NORM.DIST", L"NORM.INV", L"NORM.S.DIST", L"NORM.S.INV", L"PERCENTILE.EXC",
					L"PERCENTILE.INC", L"PERCENTRANK.EXC", L"PERCENTRANK.INC", L"POISSON.DIST", L"QUARTILE.EXC", L"QUARTILE.INC",
					L"RANDARRAY", L"RANK.AVG", L"RANK.EQ", L"SEQUENCE", L"SORTBY", L"STDEV.P", L"STDEV.S", L"SWITCH", L"T.DIST",
					L"T.DIST.2T", L"T.DIST.RT", L"T.INV", L"T.INV.2T", L"T.TEST", L"TAKE", L"TEXTAFTER", L"TEXTBEFORE",
					L"TEXTJOIN", L"TEXTSPLIT", L"TOCOL", L"TOROW", L"UNIQUE", L"VAR.P", L"VAR.S", L"VSTACK", L"WEBSERVICE",
					L"WEIBULL.DIST", L"WRAPCOLS", L"WRAPROWS", L"XLOOKUP", L"XMATCH", L"Z.TEST"});
				add_names(n, L"COM.MICROSOFT.", L"_xlfn._xlws.", {L"FILTER", L"SORT"});
				add_names(n, L"COM.MICROSOFT.", L"", {L"BAHTTEXT", L"CEILING", L"FLOOR", L"ISO.CEILING", L"NETWORKDAYS.INTL", L"WORKDAY.INTL"});
				add_names(n, L"LEGACY.", L"", {
					L"CHIDIST", L"CHIINV", L"CHITEST", L"FDIST", L"FINV", L"NORMSDIST", L"NORMSINV", L"TDIST"});
				// OpenFormula functions that Excel added later and stores with the _xlfn. prefix
				add_names(n, L"", L"_xlfn.", {
					L"ACOT", L"ACOTH", L"ARABIC", L"BASE", L"BINOM.DIST.RANGE", L"BITAND", L"BITLSHIFT", L"BITOR", L"BITRSHIFT",
					L"BITXOR", L"COMBINA", L"COT", L"COTH", L"CSC", L"CSCH", L"DAYS", L"DECIMAL", L"GAMMA", L"GAUSS", L"IFNA",
					L"IMCOSH", L"IMCOT", L"IMCSC", L"IMCSCH", L"IMSEC", L"IMSECH", L"IMSINH", L"IMTAN", L"ISFORMULA",
					L"ISOWEEKNUM", L"MUNIT", L"NUMBERVALUE", L"PDURATION", L"PERMUTATIONA", L"PHI", L"RRI", L"SEC", L"SECH",
					L"SHEET", L"SHEETS", L"UNICHAR", L"UNICODE", L"XOR"});
				n[L"FDIST"]		= L"_xlfn.F.DIST";
				n[L"FINV"]		= L"_xlfn.F.INV";
				n[L"FORMULA"]	= L"_xlfn.FORMULATEXT";
				n[L"SKEWP"]		= L"_xlfn.SKEW.P";
				return n;
			}();
			return names;
		}

		std::wstring bare_ooxml_name(std::wstring name)
		{
			for (const std::wstring prefix : {L"_XLFN.", L"_XLWS."})
			{
				if (boost::algorithm::starts_with(name, prefix))
					name.erase(0, prefix.size());
			}
			return name;
		}

		const names_map& odf_by_bare_ooxml_name()
		{
			static const names_map names = []
			{
				names_map n;
				for (const auto& name : ooxml_by_odf_name())
					n[bare_ooxml_name(boost::algorithm::to_upper_copy(name.second))] = name.first;
				return n;
			}();
			return names;
		}

		// A name directly before "(", not part of a longer name, a sheet reference or an ODF condition like is-true-formula(
		const boost::wregex function_name_re(L"(?<![\\w.$])(?!(?<=[a-z]-)[a-z]+\\()[A-Za-z_][\\w.]*(?=\\()");

		std::wstring odf2oox_function_name(const boost::wsmatch& what)
		{
			const std::wstring name = boost::algorithm::to_upper_copy(what.str());
			const names_map& names = ooxml_by_odf_name();

			auto it = names.find(name);
			if (it != names.end())
				return it->second;

			// Take one to three arguments, the editor's CEILING and FLOOR exactly two. Import only:
			// the editor's CEILING and FLOOR export as COM.MICROSOFT.CEILING and COM.MICROSOFT.FLOOR
			if (name == L"CEILING" || name == L"FLOOR")
				return L"_xlfn." + name + L".MATH";

			// Functions LibreOffice learns later get the same prefixes
			if (boost::algorithm::starts_with(name, L"COM.MICROSOFT."))
				return L"_xlfn." + name.substr(14);
			if (boost::algorithm::starts_with(name, L"LEGACY."))
				return name.substr(7);

			return what.str();
		}

		std::wstring oox2odf_function_name(const boost::wsmatch& what)
		{
			const names_map& names = odf_by_bare_ooxml_name();

			auto it = names.find(bare_ooxml_name(boost::algorithm::to_upper_copy(what.str())));
			return it != names.end() ? it->second : what.str();
		}
	}

	std::wstring odf2oox_function_names(const std::wstring& expr)
	{
		return boost::regex_replace(expr, function_name_re, &odf2oox_function_name, boost::match_default | boost::format_all);
	}

	std::wstring oox2odf_function_names(const std::wstring& expr, void (*mask)(std::wstring&))
	{
		return boost::regex_replace(expr, function_name_re, [mask](const boost::wsmatch& what)
		{
			std::wstring name = oox2odf_function_name(what);
			if (mask)
				mask(name);
			return name;
		}, boost::match_default | boost::format_all);
	}

	class odf2oox_converter::Impl
	{
	public:
		Impl() {}
		
		static std::unordered_map<std::wstring, int> & mapExternalLink_;

		std::wstring convert(const std::wstring& expr);
		std::wstring convert_chart_distance(const std::wstring& expr);

		std::wstring convert_list_values(const std::wstring& expr);

		void split_distance_by(const std::wstring& expr, const std::wstring& by, std::vector<std::wstring>& out);
		
		std::wstring replace_cells_range(std::wstring expr, bool withTableName, bool bAbsoluteAlways = false);
		bool check_formula(std::wstring& expr);
		void replace_semicolons(std::wstring& expr, bool del_quotes = false);
		void replace_tilda(std::wstring& expr);
		void replace_vertical(std::wstring& expr);
		void replace_space(std::wstring& expr);
		void replace_apersand(std::wstring& expr);
	
		std::wstring convert_named_ref(const std::wstring& expr, bool withTableName, std::wstring separator, bool bAbsoluteAlways);
		std::wstring convert_named_expr(const std::wstring& expr, bool withTableName, bool bAbsoluteAlways);
		
		static std::wstring replace_named_ref_formater(boost::wsmatch const & what);
		static std::wstring replace_named_ref_formater_2(boost::wsmatch const & what);
		//static std::wstring replace_cell_range_formater(boost::wsmatch const & what);
		
		void replace_named_formula(std::wstring & expr, bool w = true);
		void replace_named_ref(std::wstring & expr, bool withTable = true, bool bAbsoluteAlways = false);
		
		bool find_first_ref(std::wstring const & expr, std::wstring & table,  std::wstring & ref);
		bool find_first_last_ref(std::wstring const & expr, std::wstring & table, std::wstring & ref_first,std::wstring & ref_last);
	
		static bool				convert_with_absolute;
		static bool				convert_with_TableName;
		static std::wstring		table_name_;
		static std::vector<std::map<std::wstring, std::wstring>> mapReplacements;
//-------------------------------------------------------------------------------------------------------------
		static std::wstring replace_apersand_formater(boost::wsmatch const & what)
		{
			if (what[1].matched)
				return L"&amp;";
			else if (what[2].matched)
				return what[2].str();
			else if (what[3].matched)
				return what[3].str();
			else
				return L"";
		}
		static std::wstring replace_semicolons_formater_del(boost::wsmatch const & what)
		{
			if (what[1].matched)
				return L",";
			else if (what[2].matched)
			{
				std::wstring res = what[2].str();
				if (res.length() > 1 && res[0] == L'\"')
					res = res.substr(1, res.length() - 2);
				return res;
			}
			else
				return L"";
		}
		static std::wstring replace_semicolons_formater(boost::wsmatch const & what)
		{
			if (what[1].matched)
				return L",";
			else if (what[2].matched)
				return what[2].str();    
			else if (what[3].matched)
				return what[3].str();
			else
				return L"";
		}
		static std::wstring replace_tilda_formater(boost::wsmatch const & what)
		{
			if (what[1].matched)
				return L";";
			else if (what[2].matched)
				return what[2].str();    
			else if (what[3].matched)
				return what[3].str();
			else
				return L"";
		}
		static std::wstring replace_vertical_formater(boost::wsmatch const & what)
		{
			if (what[1].matched)
			{
				std::wstring inner = what[1].str();
				XmlUtils::replace_all( inner, L"|", L";");
				return L"{" + inner + L"}";
			}    
			else if (what[2].matched)
				return what[2].str();
			else if (what[3].matched)
				return what[3].str();
		
			return L"";
		}

		static void replace_tmp_back(std::wstring &expr)
		{
			for (auto key : mapReplacements.back())
			{
				XmlUtils::replace_all(expr, key.first, key.second);
			}
		}
		static void replace_tmp(std::wstring &expr)
		{
			//std::random_device genSource;
			//std::uniform_int_distribution<> generator(0, 23);
			//for (int index = 0; index < 5; index++)
			//{
			//	key += wchar_t(L'a' + generator(genSource));
			//}

			std::wstring key = L"aaaaaaaaaaaaaaaaaaaaaaaa";
			for (unsigned i = 0; i < 23; ++i)
			{
				unsigned j = rand() % (i + 1);
				key[i] = key[j];
				key[j] = wchar_t(L'a' + i);
			}
			mapReplacements.back().insert(std::make_pair(key, expr));
			expr = key;
		}
		static std::wstring convert_scobci(boost::wsmatch const & what)
		{
			if (what[1].matched)
			{
				std::wstring inner = what[1].str();

				replace_tmp(inner);

				return inner;
			}
			else if (what[2].matched)
			{
				std::wstring inner = what[2].str();
				
				replace_tmp(inner);

				return inner;
			}
			else if (what[3].matched)
				return what[3].str();
		
			return L"";
		}
		static std::wstring replace_space_formater(boost::wsmatch const & what)
		{
			if (what[1].matched)
			{
				std::wstring inner = what[1].str();
				XmlUtils::replace_all( inner, L" ", L",");
				return inner;
			}    
			else if (what[2].matched)
				return what[2].str();
			else if (what[3].matched)
				return what[3].str();

			return L"";
		}


	};
	bool			odf2oox_converter::Impl::convert_with_TableName = true;
	std::wstring	odf2oox_converter::Impl::table_name_			= L"";
	bool			odf2oox_converter::Impl::convert_with_absolute	= false;
	std::vector<std::map<std::wstring, std::wstring>> odf2oox_converter::Impl::mapReplacements;

	std::unordered_map<std::wstring, int> &odf2oox_converter::Impl::mapExternalLink_ = oox::xlsx_conversion_context::mapExternalLink_;

	bool odf2oox_converter::Impl::find_first_last_ref(std::wstring const & expr, std::wstring & table,std::wstring & ref_first,std::wstring & ref_last)
	{	
		std::wstring workstr = expr;
		
		mapReplacements.emplace_back();

		workstr = boost::regex_replace(
			workstr,
			boost::wregex(L"('.*?')|(\".*?\")"),
			&convert_scobci, boost::match_default | boost::format_all);

		std::vector< std::wstring > split;
	    
		boost::algorithm::split(split, workstr, boost::algorithm::is_any_of(L".:"), boost::algorithm::token_compress_on);
	  
		bool res = false;
		if (split.size() == 2)
		{
			table		= split[0];	
			ref_first	= split[1];
			ref_last	= split[1];
			res = true;
		}
		if (split.size() == 3)
		{
			table		= split[0];	
			ref_first	= split[1];
			ref_last	= split[2];
			res = true;
		}
		if (split.size() == 4)
		{
			table		= split[0];	
			ref_first	= split[1];
			ref_last	= split[3];
			res = true;
		}
		replace_tmp_back( table );

		mapReplacements.pop_back();
		return res;
	}

	bool odf2oox_converter::Impl::find_first_ref(std::wstring const & expr, std::wstring & table, std::wstring & ref)
	{
                boost::wregex re(L"\\[(?:\\$)?([^\\.]+?){0,1}\\.([\\w^0-9\\$]+\\d+)(?::\\.([\\w^0-9]+\\d+)){0,1}\\]");
		boost::wsmatch result;
		boost::regex_search(expr, result, re);

		size_t sz = result.size();
		if (sz == 4 && !result[1].matched)
		{
			table = L"";
			ref =  result[2].str();        
			return true;
		}
		else if (sz == 4 && result[1].matched)
		{
			table = result[1].str();
			XmlUtils::replace_all( table, L"$", L"");
			XmlUtils::replace_all( table, L"'", L"");
			ref =  result[2].str();                
			return true;
		}
		return false;
	}

	
	//std::wstring odf2oox_converter::Impl::replace_cell_range_formater(boost::wsmatch const & what)
	//{
	//	const size_t sz = what.size();
	//	if (sz == 4 && !what[1].matched)
	//	{
	//		const std::wstring c1 = what[2].str(); 
	//		const std::wstring c2 = what[3].str(); 
	//		const std::wstring s =  c1 + (c2.empty() ? L"" : (L":" + c2) );
	//		return s;
	//	}
	//	else if (sz == 4 && what[1].matched)
	//	{
	//		std::wstring sheet1 = what[1].str();
	//		XmlUtils::replace_all( sheet1, L"$", L"");

	//		const std::wstring c1 = what[2].str(); 
	//		std::wstring c2 = what[3].str(); 
	//		if (c2.empty()) c2 = what[4].str(); 
	//		const std::wstring s =  sheet1 + L"!" + c1 + (c2.empty() ? L"" : (L":" + c2) );
	//		return s;
	//	}
	//	return L"";
	//}

	std::wstring odf2oox_converter::Impl::replace_named_ref_formater(boost::wsmatch const & what)
	{
		const size_t sz = what.size();

		if (sz < 6) return what[0].str();

		std::wstring external	= sz == 7 ? what[1].matched ? what[1].str() : what[2].str() : what[1].str();
		std::wstring sheet1		= sz == 7 ? what[3].str() : what[2].str();
		std::wstring ref1		= sz == 7 ? what[4].str() : what[3].str();
		std::wstring ref2		= sz == 7 ? what[6].str() : what[5].str();
		
		XmlUtils::replace_all( sheet1, L"$", L"");

		if (!external.empty())
		{
			replace_tmp_back(external);

			int id = -1;//add_external_link(external);

			if (external[0] == L'\'') external = external.substr(1, external.size() - 2);

			std::unordered_map<std::wstring, int>::iterator pFind = mapExternalLink_.find(external);
			if ( pFind == mapExternalLink_.end())
			{
				id = (int)mapExternalLink_.size() + 1;
				mapExternalLink_.insert(std::make_pair(external, id));
			}
			else
			{
				id = pFind->second;
			}
			if (sheet1[0] == L'\'')
			{
				sheet1 = sheet1.substr(1, sheet1.length() - 2);
			}
			replace_tmp_back(sheet1);
			if (sheet1[0] == L'\'') sheet1 = sheet1.substr(1, sheet1.size() - 2);

			sheet1 = L"'[" + std::to_wstring(id) + L"]" + sheet1 + L"'";
		}
		else if (std::wstring::npos != sheet1.find(L" "))
		{
			if (sheet1[0] != L'\'')
			{
				sheet1 = L"'" + sheet1 + L"'";
			}
		}
		
		table_name_ = sheet1;

		if (convert_with_absolute)
		{
			size_t col = 0, row = 0;
			oox::getCellAddressInv(ref1, col, row);

			ref1 = oox::getCellAddress(col, row, true);

			if (!ref2.empty())
			{
				oox::getCellAddressInv(ref2, col, row);
				ref2 = oox::getCellAddress(col, row, true);
			}
		}
		
		if (convert_with_TableName)
		{
			return (sheet1.empty() ? L"" : (sheet1 + L"!")) + ref1 + (ref2.empty() ? L"" : (L":" + ref2) );
		}
		else
		{
			return ref1 + (ref2.empty() ? L"" : (L":" + ref2) );
		}
	}
	std::wstring odf2oox_converter::Impl::replace_named_ref_formater_2(boost::wsmatch const & what)
	{
		//if (what.size() < 6) return what[0].str();

		std::wstring sheet1 = what[1].str();
		std::wstring ref1 = what[2].str();
		std::wstring ref2 = what[3].str();

		XmlUtils::replace_all(sheet1, L"$", L"");
		XmlUtils::replace_all(ref2, L":", L"");

		if (std::wstring::npos != sheet1.find(L" "))
		{
			if (sheet1[0] != L'\'')
			{
				sheet1 = L"'" + sheet1 + L"'";
			}
		}

		table_name_ = sheet1;

		if (convert_with_absolute)
		{
			size_t col = 0, row = 0;
			oox::getCellAddressInv(ref1, col, row);

			ref1 = oox::getCellAddress(col, row, true);

			if (!ref2.empty())
			{
				oox::getCellAddressInv(ref2, col, row);
				ref2 = oox::getCellAddress(col, row, true);
			}
		}

		if (convert_with_TableName)
		{
			return (sheet1.empty() ? L"" : (sheet1 + L"!")) + ref1 + (ref2.empty() ? L"" : (L":" + ref2));
		}
		else
		{
			return ref1 + (ref2.empty() ? L"" : (L":" + ref2));
		}
	}


	// replacing the cell addressing format
	// [.A1] -> A1
	// [.A1:.B5] -> A1:B5
	// [Sheet2.A1:B5] -> Sheet2!A1:B5
	// [Sheet2.A1] -> Sheet2!A1
	// [$'Sheet2 A'.$B2] -> 'Sheet2 A'!$B2
	std::wstring odf2oox_converter::Impl::replace_cells_range(std::wstring expr, bool withTableName, bool bAbsoluteAlways)
	{
		if (expr.empty()) return L"";

		XmlUtils::replace_all( expr, L"#REF !", L"#REF!");
		XmlUtils::replace_all( expr, L"#REF!#REF!", L"#REF!");
		XmlUtils::replace_all( expr, L"$#REF!$#REF!", L"#REF!");

		convert_with_TableName = withTableName;
		convert_with_absolute = bAbsoluteAlways;

		//boost::wregex complexRef(L"\\[(?:\'([^\']*)\'#){0,1}\\[{0,1}(?:\\$){0,1}([^\\.]+?){0,1}\\.(\\${0,1}[\\w^0-9]*\\${0,1}\\d*)(?::(\\${0,1}[^\\.]+?){0,1}\\.(\\${0,1}[\\w^0-9]*\\${0,1}\\d*)){0,1}\\]{0,1}");
		boost::wregex complexRef(L"(?:(?:(?:(?:\\[(.*)#)|(?:(.*)#\\[)))|(?:\\[))\
(?:\\$){0,1}([^\\.]+?){0,1}\\.(\\${0,1}[\\w^0-9]*\\${0,1}\\d*)(?::(\\${0,1}[^\\.]+?){0,1}\\.(\\${0,1}[\\w^0-9]*\\${0,1}\\d*)){0,1}\\]");
//									 [ external#  [  $   Sheet2         . A1								 : ( $   Sheet2)? . B5                    ]

		std::wstring result = boost::regex_replace(
  			expr,
			complexRef,
			&replace_named_ref_formater,
			boost::match_default | boost::format_all);

		if (result == expr)
		{
			//bad formula ???
			boost::wregex complexRef_2(L"\\$([^\\.]+?)?\\.(\\$[a-zA-Z]*\\d*)(\\:\\$[a-zA-Z]*\\d*)?");
			//	$ Sheet2  . $ A1		 : ( $   Sheet2)? . $ B5   

			result = boost::regex_replace(
				expr,
				complexRef_2,
				&replace_named_ref_formater_2,
				boost::match_default | boost::format_all);
		}
		return result;
	}
	void odf2oox_converter::Impl::replace_named_ref(std::wstring & expr, bool withTableName, bool bAbsoluteAlways)
	{
		if (expr.empty()) return;

		XmlUtils::replace_all( expr, L"#REF !", L"#REF!");
		XmlUtils::replace_all( expr, L"#REF!#REF!", L"#REF!");
		XmlUtils::replace_all( expr, L"$#REF!$#REF!", L"#REF!");

		convert_with_TableName = withTableName;
		convert_with_absolute = bAbsoluteAlways;
		
		//boost::wregex complexRef(L"\\${0,1}([^\\.]+?){0,1}\\.(\\${0,1}[a-zA-Z]+\\${0,1}\\d+)(?::\\.(\\${0,1}[a-zA-Z]+\\${0,1}\\d+)){0,1}");
		boost::wregex complexRef(L"\\[{0,1}(?:(.*)#){0,1}\\${0,1}([^\\.\\s]+?){0,1}\\.(\\${0,1}[\\w^0-9]*\\${0,1}\\d*)(?::\\${0,1}([^\\.\\s]+?){0,1}\\.(\\${0,1}[\\w^0-9]*\\${0,1}\\d*)){0,1}\\]{0,1}");
//									  external#  $   Sheet2         . A1								 : ( $   Sheet2)? . B5   
	
		const std::wstring res = boost::regex_replace(
			expr,
			complexRef,
			&replace_named_ref_formater,
			boost::match_default | boost::format_all);
		expr = res;    
	}

	// recognize and replace formula format
	// of:=(Formula) -> (Formula)
	bool odf2oox_converter::Impl::check_formula(std::wstring& expr)
	{
		if (expr.empty()) return false;

		boost::match_results<std::wstring::const_iterator> res;
		if (boost::regex_search(expr, res, boost::wregex(L"^(?:[\\w]+:)?=(.+)"), boost::match_default))
		{
			expr = res[1].str();
			while (expr.find(L"=") == 0)
			{
				expr.erase(expr.begin(), expr.begin() + 1);
			}
			return true;
		}
		else   
			return false;
	}
// replace semicolons in all occurrences except those in quotes --* and in curly braces*--
	void odf2oox_converter::Impl::replace_semicolons(std::wstring& expr, bool del_quotes)
	{
		 const std::wstring res = boost::regex_replace(
			expr,
			//boost::wregex(L"(;)|(?:\".*?\")|(?:'.*?')"),
			boost::wregex(L"(;)|(\".*?\")|('.*?')"),
			del_quotes ? &replace_semicolons_formater_del : &replace_semicolons_formater,
			boost::match_default | boost::format_all);

		 expr = res;
	}
	void odf2oox_converter::Impl::replace_tilda(std::wstring& expr)
	{
		 const std::wstring res = boost::regex_replace(
			expr,
			//boost::wregex(L"(;)|(?:\".*?\")|(?:'.*?')"),
			boost::wregex(L"(~)|(\".*?\")|('.*?')"),
			&replace_semicolons_formater,
			boost::match_default | boost::format_all);

		 expr = res;
	}
	void odf2oox_converter::Impl::replace_apersand(std::wstring& expr)
	{
		const std::wstring res = boost::regex_replace(
			expr,
			//boost::wregex(L"(;)|(?:\".*?\")|(?:'.*?')"),
			boost::wregex(L"(&)|(\".*?\")|('.*?')"),
			&replace_apersand_formater,
			boost::match_default | boost::format_all);

		expr = res;
	}
	// replace vertical bar in all occurrences within curly braces, but not inside strings
	void odf2oox_converter::Impl::replace_vertical(std::wstring& expr)
	{
		 const std::wstring res = boost::regex_replace(
			expr,
			boost::wregex(L"(?:(?:\\{)([^\\}]*?)(?:\\}))|(\".*?\")|('.*?')"),
			&replace_vertical_formater,
			boost::match_default | boost::format_all);
		 expr = res;
	}
	// replace the space in all occurrences with a comma
	void odf2oox_converter::Impl::replace_space(std::wstring& expr)
	{
		 const std::wstring res = boost::regex_replace(
			expr,
			boost::wregex(L"(?:(?:\\{)([^\\}]*?)(?:\\}))|(\".*?\")|('.*?')"),
			&replace_space_formater,
			boost::match_default | boost::format_all);
		 expr = res;
	}

	std::wstring odf2oox_converter::Impl::convert(const std::wstring& expr)
	{
		std::wstring workstr = expr;

		bool isFormula = check_formula(workstr);

//escaping
		mapReplacements.emplace_back();

		workstr = boost::regex_replace(workstr,
			boost::wregex(L"('.*?')|(\".*?\")"),
			&convert_scobci, boost::match_default | boost::format_all);

		workstr = replace_cells_range	(workstr, true);
		replace_semicolons	(workstr);
		replace_tilda		(workstr);
		replace_vertical	(workstr);
		//replace_apersand	(workstr);
		workstr = odf2oox_function_names(workstr);

		if (isFormula)
		{
			// ROUNDUP( - there are 2 parameters in oox - the bit width is needed - ,0) - EV Requirements v2.2.3.ods
			
			if (std::wstring::npos != workstr.find(L"CONCATINATE"))
			{
				bool l = true;
				//ranges can be partially specified
				//TODO
			}	
			//TODO INDEX((A1:C6~A8:C11),2,2,2) - ???? - INDEX_emb.ods
		}
		else
		{
			size_t sz_workstr = workstr.length();
            if (workstr.substr(0, (std::min)((size_t)3, sz_workstr)) == L"of:")//sample_02neu_crashes.ods
				workstr =  workstr.substr(3);
		}


	//-----------------------------------------------------------
		replace_tmp_back(workstr);

		mapReplacements.pop_back();
		return workstr;
	}

	void odf2oox_converter::Impl::split_distance_by(const std::wstring& expr, const std::wstring& by, std::vector<std::wstring>& out)
	{
		mapReplacements.emplace_back();

		std::wstring workstr = boost::regex_replace(
			expr,
			boost::wregex(L"('.*?')|(\".*?\")"),
			&convert_scobci, boost::match_default | boost::format_all);
	    
		boost::algorithm::split(out, workstr, boost::algorithm::is_any_of(by), boost::algorithm::token_compress_on);
		
		for (size_t i = 0; i < out.size(); i++)
		{
			replace_tmp_back(out[i]);
		}
		mapReplacements.pop_back();
	}

	//Sheet2.C3:Sheet2.C19 Sheet2.L29:Sheet2.L36
	//in
	//Sheet2!C3:C19,Sheet2!L27:L34
	std::wstring odf2oox_converter::Impl::convert_list_values(const std::wstring& expr)
	{
		std::wstring workstr = expr;
		replace_semicolons(workstr, true);
		return workstr;
	}

	std::wstring odf2oox_converter::Impl::convert_chart_distance(const std::wstring& expr)
	{
		mapReplacements.emplace_back();

		std::wstring workstr = boost::regex_replace(
			expr,
			boost::wregex(L"('.*?')|(\".*?\")"),
			&convert_scobci, boost::match_default | boost::format_all);
	    
		std::vector<std::wstring> distance_inp;
		std::vector<std::wstring> distance_out;

		boost::algorithm::split(distance_inp, workstr, boost::algorithm::is_any_of(L" "), boost::algorithm::token_compress_on);

		for (size_t i = 0; i < distance_inp.size(); i++)
		{
			std::wstring sheet;
			std::vector<std::wstring> range;
			std::vector<std::wstring> cells;

			boost::algorithm::split(range, distance_inp[i], boost::algorithm::is_any_of(L":"), boost::algorithm::token_compress_on);

			for (size_t j = 0; j < range.size(); j++)
			{
				const std::string::size_type colon = range[j].find('.');

				if (colon != std::wstring::npos)
				{
					cells.push_back(range[j].substr(colon + 1));

					if (sheet.empty())
					{
						sheet = range[j].substr(0, colon);
					}
				}
				else
					cells.push_back(range[j]);
			}
			std::wstring cells_out;
			for (size_t j = 0; j < cells.size(); j++)
			{
				cells_out.append(cells[j]);
				cells_out.append(L":");
			}
			
			if (sheet.find(L"-") != std::wstring::npos && sheet.find(L"'") == std::wstring::npos && sheet.find(L"\"") == std::wstring::npos)
			{
				sheet = L"'" + sheet + L"'";
			}

			distance_out.push_back((sheet.empty() ? L"" : (sheet + L"!")) + cells_out.substr(0, cells_out.size() - 1));
		}
		std::wstring result;

		for (size_t i = 0; i < distance_out.size(); i++)
		{
			result.append(distance_out[i]);
			result.append(L",");
		}
 		replace_tmp_back( result );
		
		mapReplacements.pop_back();
		return result.substr(0, result.size() - 1);// minus the last extra comma
	}
	std::wstring odf2oox_converter::Impl::convert_named_ref(const std::wstring& expr, bool withTableName, std::wstring separator, bool bAbsoluteAlways)
	{
		boost::wregex complexRef(L"('(?!\\s\\'){0,1}.*?')");// search for what is in apostrophes and replace there

		std::wstring workstr = expr;

		mapReplacements.emplace_back();

		workstr = boost::regex_replace(
			workstr,
			boost::wregex(L"('.*?')|(\".*?\")"),
			&convert_scobci, boost::match_default | boost::format_all);

		replace_named_ref(workstr, withTableName, bAbsoluteAlways);

		if (separator != L" ")
		{
			XmlUtils::replace_all( workstr, L" ", separator);
		}

		replace_tmp_back( workstr );

		if (!table_name_.empty())
		{
			replace_tmp_back( table_name_ );
		}
		mapReplacements.pop_back();
		return workstr;
	}
	std::wstring odf2oox_converter::Impl::convert_named_expr(const std::wstring& expr, bool withTableName, bool bAbsoluteAlways)
	{
		std::wstring workstr = expr;

		bool isFormula = check_formula(workstr);

		if (isFormula)
		{
			workstr = convert(expr);
		}
		else
		{
			mapReplacements.emplace_back();

			workstr = boost::regex_replace(
				workstr,
				boost::wregex(L"('.*?')|(\".*?\")"),
				&convert_scobci, boost::match_default | boost::format_all);
		   
			workstr = replace_cells_range(workstr, withTableName, bAbsoluteAlways);
			replace_semicolons(workstr);
			replace_vertical(workstr);
			workstr = odf2oox_function_names(workstr);

			if (workstr.find(L"CONCATINATE") != std::wstring::npos)
			{
				//ranges can be partially specified
				//TODO

			}
			replace_tmp_back(workstr);

			if (!table_name_.empty())
			{
				replace_tmp_back(table_name_);
 			}
			mapReplacements.pop_back();
		}
		return workstr;
	}

//------------------------------------------------------------------------------------------------------------
	odf2oox_converter::odf2oox_converter() : impl_(new odf2oox_converter::Impl())
	{
	}

	odf2oox_converter::~odf2oox_converter()
	{
	}

	std::wstring odf2oox_converter::get_table_name()
	{
		return impl_->table_name_;
	}
	std::wstring odf2oox_converter::convert_list_values(std::wstring const & expr)
	{
		return impl_->convert_list_values(expr);
	}
	std::wstring odf2oox_converter::convert(const std::wstring& expr)
	{
		return impl_->convert(expr);
	}
	std::wstring odf2oox_converter::convert_chart_distance(const std::wstring& expr)
	{
		return impl_->convert_chart_distance(expr);
	}
	void odf2oox_converter::split_distance_by(const std::wstring& expr, const std::wstring& by, std::vector<std::wstring>& out)
	{
		return impl_->split_distance_by(expr, by, out);
	}
	std::wstring odf2oox_converter::convert_named_ref(const std::wstring& expr, bool withTableName, std::wstring separator, bool bAbsoluteAlways)
	{
		return impl_->convert_named_ref(expr, withTableName, separator, bAbsoluteAlways);
	}
	std::wstring odf2oox_converter::convert_named_expr(const std::wstring& expr, bool withTableName, bool bAbsoluteAlways)
	{
		return impl_->convert_named_expr(expr, withTableName, bAbsoluteAlways);
	}

	std::wstring odf2oox_converter::convert_ref(std::wstring const & expr)
	{
		std::wstring workstr = L"[" + expr + L"]";
		return impl_->replace_cells_range(workstr, true);
	}
	std::wstring odf2oox_converter::convert_spacechar(std::wstring expr)
	{
		while(true)
		{
			size_t pos = expr.find(L"%20");
			if (pos == std::wstring::npos)break;

			expr.replace(pos,3,L" ");
		}
		return expr; 
	}

	bool odf2oox_converter::find_first_ref(std::wstring const & expr, std::wstring & table, std::wstring & ref)
	{
		return impl_->find_first_ref(expr, table, ref);
	}
	bool odf2oox_converter::find_first_last_ref(std::wstring const & expr, std::wstring & table, std::wstring & ref_first,std::wstring & ref_last)
	{
		return impl_->find_first_last_ref(expr,  table, ref_first, ref_last);
	}

}
}
