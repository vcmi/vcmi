#ifndef __GENERAL_AI_H__
#define __GENERAL_AI_H__

#include "Common.h"

namespace GeniusAI { namespace GeneralAI {

	class CGeneralAI
	{
	public:
		CGeneralAI();
		~CGeneralAI();

		void init(ICallback* CB);
	private:
		ICallback *m_cb;
	};

}}

#endif/*__GENERAL_AI_H__*/