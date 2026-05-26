#include "Table.h"

namespace NSDocxRenderer
{
	namespace NSTables
	{
		ITableBuilder* Create()
		{
			return new ITableBuilder();
		}
	}
}
