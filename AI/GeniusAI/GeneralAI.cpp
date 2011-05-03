#include "GeneralAI.h"
#include "../../CCallback.h"

using namespace geniusai::GeneralAI;

CGeneralAI::CGeneralAI()
	: m_cb(NULL)
{
}

CGeneralAI::~CGeneralAI()
{
}

void CGeneralAI::init(CCallback *CB)
{
	assert(CB != NULL);
	m_cb = CB;
	CB->waitTillRealize = true;
}