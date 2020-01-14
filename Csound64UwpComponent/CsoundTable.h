#pragma once
#include "CsoundTable.g.h"
#include "Csound.h"
#include <vector>

namespace winrt::Csound64UwpComponent::implementation
{
    struct CsoundTable : CsoundTableT<CsoundTable>
    {
        CsoundTable() = default;

		int32_t Number();
		int32_t Size();
        int32_t GenNbr();
		bool IsNormalized();
		com_array<double> Parameters(); //As returned from csound, includes GenNbr [0], [1..n] are parameters
        com_array<double> Contents();
        void Contents(array_view<double const> value);


		/***********  Internal (Public) Code that is used within Csound64 Module but not part of public metadata in winmd file.   ************/
		void GetTableContents(CSOUND* pCsound, int32_t tableNumber);
		void UpdateTableContents(CSOUND* pCsound);

	private:
		int32_t m_tableNumber{ 0 };
		int32_t m_tableSize{ -1 };
		std::vector<MYFLT> m_tableData{};
		std::vector<MYFLT> m_tableArgs{};
    };
}
namespace winrt::Csound64UwpComponent::factory_implementation
{
    struct CsoundTable : CsoundTableT<CsoundTable, implementation::CsoundTable>
    {
    };
}
