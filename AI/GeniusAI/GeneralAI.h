#pragma once

#include "Common.h"

namespace geniusai { namespace GeneralAI {

	class CGeneralAI
	{
	public:
		CGeneralAI();
		~CGeneralAI();

		void init(CCallback* CB);
	private:
		CCallback *m_cb;
	};

}}