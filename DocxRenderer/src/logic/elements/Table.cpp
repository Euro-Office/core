#include "Table.h"

namespace NSDocxRenderer
{
	void ITableBuilder::Clear()
	{
		m_arTables.clear();
	}
	void ITableBuilder::SetShapes(std::vector<shape_ptr_t>&& shapes)
	{
		m_arShapes = std::move(shapes);
	}
	void ITableBuilder::SetParagraphs(std::vector<paragraph_ptr_t>&& paragraphs)
	{
		m_arParagaraphs = std::move(paragraphs);
	}

	std::vector<ITableBuilder::shape_ptr_t>&& ITableBuilder::ReturnShapes()
	{
		return std::move(m_arShapes);
	}
	std::vector<ITableBuilder::paragraph_ptr_t>&& ITableBuilder::ReturnParagraphs()
	{
		return std::move(m_arParagaraphs);
	}
	std::vector<ITableBuilder::ooxml_item_ptr_t>&& ITableBuilder::GetTables()
	{
		return std::move(m_arTables);
	}

	void ITableBuilder::BuildGraphicallCells()
	{

	}
	void ITableBuilder::BuildTables()
	{

	}

	namespace NSTables
	{
		ITableBuilder Create()
		{
			return ITableBuilder();
		}
	}
}
