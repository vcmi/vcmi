#include "GeneralAI.h"

using namespace GeniusAI::GeneralAI;

CGeneralAI::CGeneralAI()
	: m_cb(NULL)
{
}

CGeneralAI::~CGeneralAI()
{
}

void CGeneralAI::init(ICallback *CB)
{
	assert(CB != NULL);
	m_cb = CB;
}