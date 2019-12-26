#include "pch.h"
#include "CsoundTable.h"
#include "CsoundTable.g.cpp"
#include <algorithm>
#include <cmath>


namespace winrt::Csound64UwpComponent::implementation
{

	int32_t CsoundTable::Number()
	{
		return m_tableNumber;
	}

    int32_t CsoundTable::Size()
    {
		return m_tableSize;
    }

    int32_t CsoundTable::GenNbr()
    {
		return (m_tableArgs.size() > 0) ? static_cast<int32_t>(std::abs(m_tableArgs[0])) : 0;
    }

	bool CsoundTable::IsNormalized()
	{
		return (m_tableArgs.size() > 0) && (m_tableArgs[0] >= 0);
	}

    com_array<double> CsoundTable::Parameters()
    {
		return com_array<double>(m_tableArgs);
    }

    com_array<double> CsoundTable::Contents()
    {
		return com_array<double>(m_tableData);
    }

    void CsoundTable::Contents(array_view<double const> values)
    {
		size_t siz = values.size();
		if (siz > m_tableData.size()) siz = m_tableData.size();
		std::copy_n(values.begin(), siz, m_tableData.begin());
    }

	void CsoundTable::GetTableContents(CSOUND* pCsound, int32_t tableNumber)
	{ 
		m_tableNumber = tableNumber;
		if (pCsound != nullptr)
		{
			m_tableSize = csoundTableLength(pCsound, tableNumber);
			if (m_tableSize > 0)
			{

				m_tableData.resize(m_tableSize);
				csoundTableCopyOut(pCsound, tableNumber, m_tableData.data());
			}

			MYFLT* pArgs = nullptr;
			int cnt = csoundGetTableArgs(pCsound, &pArgs, tableNumber);
			if (cnt > 0)
			{
				m_tableArgs.resize(cnt);
				std::copy_n(pArgs, cnt, m_tableArgs.begin());
			}
		}
	}

	void CsoundTable::UpdateTableContents(CSOUND* pCsound)
	{
		csoundTableCopyIn(pCsound, m_tableNumber, m_tableData.data());
	}
}
